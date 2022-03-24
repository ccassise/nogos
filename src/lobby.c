#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include "log.h"
#include "lobby.h"
#include "errno.h"

#define VISITED 1

/**
 * @brief Stores the current game's state. 
 * 
 */
typedef struct State {
	char turn; // The player who's turn it is.
	bool game_over; // Is the game over or not.
	Board *visted; // Used to help determine a winner.
} State;

Lobby *lobby_create(MessageQueue *msgq, size_t rows, size_t cols) {
	Lobby *l = calloc(1, sizeof *l);

	l->board = board_create(rows, cols);
	l->msgq = msgq;
	l->state = malloc(sizeof *l->state);
	l->state->turn = 'O';
	l->state->game_over = false;
	l->state->visted = board_create(rows, cols);

	return l;
}

void lobby_destroy(Lobby *l) {
	board_destroy(l->board);
	board_destroy(l->state->visted);
	free(l->state);
	free(l);
}

/**
 * @brief Fills out message data that will be broadcasted.
 * 
 * @param msg The message to fill.
 * @param str The string that will be broadcasted.
 * @param slen Length of the string that will be copied.
 */
static void fill_msg_data(Message *msg, const char *str, size_t slen) {
	msg->data_count = (int)slen;
	if (slen > 0 && str[slen - 1] == '\0') {
		msg->data_count = (int)slen - 1;
	}

	memcpy(msg->data, str, (size_t)msg->data_count);
}

/**
 * @brief Sends a message to all players in the current lobby.
 * 
 * @param l The lobby that the message will be sent to.
 * @param str The string that will be sent. If str has a terminating null
 * character it will be discarded.
 * @param slen The length of the string that will be sent.
 * @return -1 if the message failed to be put in queue. 0 if the message was
 * successfully put in queue.
 */
static int broadcast_all(Lobby *l, const char *str, size_t slen) {
	Message msg;
	
	fill_msg_data(&msg, str, slen);

	msg.to_count = l->players_count;
	// TOOD: Split in to 2 messages if players_count > MSG_MAX_RECIPIENTS.
	for (int i = 0; i < l->players_count && i < MSG_MAX_RECIPIENTS; i++) {
		msg.to[i] = l->players[i].fd;
	}

	return msgq_put(l->msgq, &msg);
}

/**
 * @brief Sends a message to all players in the current lobby except for the
 * given player.
 * 
 * @param l The lobby that the message will be sent to.
 * @param str The string that will be sent. If str has a terminating null
 * character it will be discarded.
 * @param slen The length of the string that will be sent.
 * @param playerfd The file descriptor of the player.
 * @return -1 if the message failed to be put in queue. 0 if the message was
 * successfully put in queue.
 */
static int broadcast_from(Lobby *l, const char *str, size_t slen, int playerfd) {
	Message msg;
	
	fill_msg_data(&msg, str, slen);

	msg.to_count = 0;
	// TOOD: Split in to 2 messages if players_count > MSG_MAX_RECIPIENTS.
	for (int i = 0; i < l->players_count && i < MSG_MAX_RECIPIENTS; i++) {
		if (l->players[i].fd != playerfd) {
			msg.to[msg.to_count] = l->players[i].fd;
			msg.to_count++;
		}
	}

	return msgq_put(l->msgq, &msg);
}

/**
 * @brief Updates which team every player is on. 
 * 
 * @param l The lobby in which to update players.
 */
static void update_player_team(Lobby *l) {
	if (l->players_count == 1) {
		l->players[0].team = 'O';
	} else if (l->players_count >= 2) {
		l->players[0].team = 'O';
		l->players[1].team = 'X';
	}
}

/**
 * @brief Returns wether or not the lobby is at the maximum number of players.
 * 
 * @param l The lobby instance to check.
 * @return true 
 * @return false 
 */
static bool lobby_full(Lobby *l) {
	return l->players_count == LOBBY_MAX_PLAYERS;
}

int lobby_join(Lobby *l, int playerfd) {
	// Don't let the same player join twice.
	for (int i = 0; i < l->players_count; i++) {
		if (l->players[i].fd == playerfd) {
			DEBUG("[%d] already in lobby\n", playerfd);
			return -1;
		}
	}

	if (lobby_full(l)) {
		DEBUG("[%d] lobby full\n", playerfd);
		return -1;
	}

	Player p;
	p.fd = playerfd;

	DEBUG("[%d] joined a lobby\n", playerfd);

	l->players[l->players_count] = p;
	l->players_count++;
	update_player_team(l);

	return 0;
}

void lobby_leave(Lobby *l, int playerfd) {
	int player_index = -1;
	for (int i = 0; i < l->players_count; i++) {
		if (l->players[i].fd == playerfd) {
			player_index = i;
			break;
		}
	}

	if (player_index != -1) {
		DEBUG("[%d] left lobby\n", playerfd);
		l->players[player_index] = l->players[l->players_count - 1];
		l->players_count--;
		update_player_team(l);
	}

	const char left[] = "OPPONENT_LEFT\r\n";
	broadcast_from(l, left, sizeof left / sizeof left[0], playerfd);
}

static char next_team_turn(State *s) {
	if (s->turn == 'O') {
		return 'X';
	}
	return 'O';
}

/**
 * @brief Checks all connected pieces of the given piece and return if it has
 * any liberties. A liberty is when a piece is adjacent to an empty board space
 * or if it's connected to a piece of the same team that has a liberty.
 * 
 * @param b The board to check.
 * @param team The team the check liberties for.
 * @param pos The position that will be checked.
 * @return true 
 * @return false 
 */
static bool has_liberty(Board *b, Board *visited, char team, BoardPos pos) {
	bool result = false;

	if (board_get(b, pos) == BOARD_EMPTY_SPACE) {
		return true;
	}

	if (board_get(b, pos) != team || board_get(visited, pos) == VISITED) {
		return false;
	}

	board_set(visited, VISITED, pos);

	const BoardPos adjacents[] = {
		{ .row = pos.row - 1, .col = pos.col }, // up
		{ .row = pos.row + 1, .col = pos.col }, // down
		{ .row = pos.row, .col = pos.col - 1 }, // left
		{ .row = pos.row, .col = pos.col + 1 }, // right
	};

	for (size_t i = 0; i < sizeof adjacents / sizeof adjacents[0]; i++) {
		// No need to check if they're negative since they are unsigned and will wrap.
		if (adjacents[i].row < b->rows && adjacents[i].col < b->cols) {
			result = has_liberty(b, visited, team, adjacents[i]) || result;
		}
	}

	return result;
}

/**
 * @brief Replaces all the visited spaces to their original unvisited state.
 * 
 * @param visited The board used to keep track of what spaces were visited
 * when checking for a winner.
 */
static void reset_visited(Board *visited) {
	// TODO: When the move history is implemented use that instead to reset all positions.
	for (size_t col = 0; col < visited->cols; col++) {
		for (size_t row = 0; row < visited->rows; row++) {
			const BoardPos pos = (BoardPos){ .row = row, .col = col };
			board_set(visited, !VISITED, pos);
		}
	}
}

/**
 * @brief Searches the entire board to find if any team has no liberties. If so
 * then that means the game is over.
 * 
 * @param l The lobby that will be searched.
 * @return char The team that has lost.
 */
static char find_loser(Lobby *l) {
	char loser = '\0';

	for (size_t row = 0; row < l->board->rows && loser == '\0'; row++) {
		for (size_t col = 0; col < l->board->cols && loser == '\0'; col++) {
			const BoardPos pos = (BoardPos){ .row = row, .col = col };
			char team;
			if ((team = board_get(l->board, pos)) != BOARD_EMPTY_SPACE && board_get(l->state->visted, pos) != VISITED) {
				const bool game_over = !has_liberty(l->board, l->state->visted, team, pos);
				if (game_over) {
					loser = board_get(l->board, pos);
				}
			}
		}
	}

	reset_visited(l->state->visted);

	return loser;
}

int lobby_play_move(Lobby *l, int playerfd, const char *row_str, const char *col_str) {
	if (!lobby_full(l)) {
		DEBUG("game has not started\n");
		return -1;
	} else if (l->state->game_over) {
		DEBUG("game is over\n");
		return -1;
	}

	Player *found = NULL;
	for (int i = 0; i < l->players_count; i++) {
		if (l->players[i].fd == playerfd) {
			found = &l->players[i];
		}
	}

	if (!found) {
		DEBUG("player not in lobby\n");
		return -1;
	} else if (found->team != l->state->turn) {
		DEBUG("not player's turn\n");
		return -1;
	}

	bool is_overflow = false;

	long row = strtol(row_str, NULL, 10);
	is_overflow = row == LONG_MAX || row == LONG_MIN;

	long col = strtol(col_str, NULL, 10);
	is_overflow = is_overflow || col == LONG_MAX || col == LONG_MIN;

	if (is_overflow || row > (long)l->board->rows || col > (long)l->board->cols) {
		DEBUG("move is out of bounds\n");
		return -1;
	}

	char buf[128];
	int buf_size = snprintf(buf, 128, "MOVE %s %s\r\n", row_str, col_str);

	const BoardPos pos = { .row = (size_t)row, .col = (size_t)col };
	const bool is_free = board_get(l->board, pos) == BOARD_EMPTY_SPACE;
	if (!is_free) {
		DEBUG("space is occupied\n");
		return -1;
	}

	board_set(l->board, found->team, pos);
	l->state->turn = next_team_turn(l->state);
	broadcast_from(l, buf, (size_t)buf_size, playerfd);

	char loser;
	if((loser = find_loser(l)) != '\0') {
		l->state->game_over = true;
		char winner = loser == 'O' ? 'X' : 'O';
		DEBUG("%c wins\n", winner);

		buf_size = snprintf(buf, 128, "WINS %c\r\n", winner);
		broadcast_all(l, buf, (size_t)buf_size);
	}

	return 0;
}
