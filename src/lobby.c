#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include "libnogo/nogo.h"

#include "errno.h"
#include "lobby.h"
#include "log.h"
#include "queue.h"

#define VISITED 1

/**
 * @brief Stores the current game's state. 
 * 
 */
typedef struct State {
	char turn; // The player who's turn it is.
	char winner; // If game_over is set then this will be set to the winning team.
	bool game_over; // Is the game over or not.
	NogoBoard *visted; // Used to help determine a winner.
} State;

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
static bool has_liberty(NogoBoard *b, NogoBoard *visited, char team, NogoBoardPos pos) {
	bool result = false;

	if (nogo_board_get(b, pos) == NOGO_BOARD_EMPTY_SPACE) {
		return true;
	}

	if (nogo_board_get(b, pos) != team || nogo_board_get(visited, pos) == VISITED) {
		return false;
	}

	nogo_board_set(visited, VISITED, pos);

	const NogoBoardPos adjacents[] = {
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
static void reset_visited(NogoBoard *visited) {
	// TODO: When the move history is implemented use that instead to reset all positions.
	for (size_t col = 0; col < visited->cols; col++) {
		for (size_t row = 0; row < visited->rows; row++) {
			const NogoBoardPos pos = (NogoBoardPos){ .row = row, .col = col };
			nogo_board_set(visited, !VISITED, pos);
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
			const NogoBoardPos pos = (NogoBoardPos){ .row = row, .col = col };
			char team;
			if ((team = nogo_board_get(l->board, pos)) != NOGO_BOARD_EMPTY_SPACE && nogo_board_get(l->state->visted, pos) != VISITED) {
				const bool game_over = !has_liberty(l->board, l->state->visted, team, pos);
				if (game_over) {
					loser = nogo_board_get(l->board, pos);
				}
			}
		}
	}

	reset_visited(l->state->visted);

	return loser;
}

/**
 * @brief Updates which team every player is on. 
 * 
 * @param l The lobby in which to update players.
 */
static void update_player_team(Lobby *l) {
	if (l->players_len == 1) {
		l->players[0].team = 'O';
	} else if (l->players_len >= 2) {
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
	return l->players_len == LOBBY_MAX_PLAYERS;
}

static char next_team_turn(State *s) {
	if (s->turn == 'O') {
		return 'X';
	}
	return 'O';
}

Lobby *lobby_create(size_t rows, size_t cols) {
	Lobby *l = calloc(1, sizeof *l);

	l->board = nogo_board_create(rows, cols);
	l->state = malloc(sizeof *l->state);
	l->state->turn = 'O';
	l->state->game_over = false;
	l->state->visted = nogo_board_create(rows, cols);

	return l;
}

void lobby_free(Lobby *l) {
	nogo_board_free(l->board);
	nogo_board_free(l->state->visted);
	free(l->state);
	free(l);
}

int lobby_join(Lobby *l, const Player *player) {
	// Don't let the same player join twice.
	for (int i = 0; i < l->players_len; i++) {
		if (l->players[i].fd == player->fd) {
			LOG_ERROR("[%s<%d>] already in lobby\n", player->name, player->fd);
			return -1;
		}
	}

	if (lobby_full(l)) {
		LOG_ERROR("[%s<%d>] lobby full\n", player->name, player->fd);
		return -1;
	}

	l->players[l->players_len] = *player;
	l->players_len++;
	update_player_team(l);

	return 0;
}

void lobby_leave(Lobby *l, const Player *player) {
	int player_index = -1;
	for (int i = 0; i < l->players_len; i++) {
		if (l->players[i].fd == player->fd) {
			player_index = i;
			break;
		}
	}

	if (player_index != -1) {
		l->players[player_index] = l->players[l->players_len - 1];
		l->players_len--;
		update_player_team(l);
	}
}

int lobby_play_move(Lobby *l, const Player *player, const char *row_str, const char *col_str) {
	if (!lobby_full(l)) {
		LOG_ERROR("game has not started\n");
		return -1;
	} else if (l->state->game_over) {
		LOG_ERROR("game is over\n");
		return -1;
	}

	Player *found = NULL;
	for (int i = 0; i < l->players_len; i++) {
		if (l->players[i].fd == player->fd) {
			found = &l->players[i];
		}
	}

	if (!found) {
		LOG_ERROR("player not in lobby\n");
		return -1;
	} else if (found->team != l->state->turn) {
		LOG_ERROR("not player's turn\n");
		return -1;
	}

	bool is_overflow = false;

	long row = strtol(row_str, NULL, 10);
	is_overflow = row == LONG_MAX || row == LONG_MIN;

	long col = strtol(col_str, NULL, 10);
	is_overflow = is_overflow || col == LONG_MAX || col == LONG_MIN;

	if (is_overflow || row > (long)l->board->rows || col > (long)l->board->cols) {
		LOG_ERROR("move is out of bounds\n");
		return -1;
	}

	const NogoBoardPos pos = { .row = (size_t)row, .col = (size_t)col };
	const bool is_free = nogo_board_get(l->board, pos) == NOGO_BOARD_EMPTY_SPACE;
	if (!is_free) {
		LOG_ERROR("space is occupied\n");
		return -1;
	}

	nogo_board_set(l->board, found->team, pos);
	l->state->turn = next_team_turn(l->state);

	char loser;
	if((loser = find_loser(l)) != '\0') {
		l->state->game_over = true;
		l->state->winner = loser == 'O' ? 'X' : 'O';
	}

	return 0;
}

int lobby_winner(Lobby *l) {
	if (l->state->game_over) {
		return l->state->winner;
	}
	return -1;
}
