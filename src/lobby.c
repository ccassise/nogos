#include <string.h>
#include <stdlib.h>

#include "log.h"
#include "lobby.h"
#include "errno.h"

Lobby *lobby_create(MessageQueue *msgq) {
	Lobby *l = calloc(1, sizeof *l);

	l->board = board_create(7, 5);
	l->msgq = msgq;

	return l;
}

void lobby_destroy(Lobby *l) {
	board_destroy(l->board);
	free(l);
}

/**
 * @brief Fills out data and data_count fields of a Message.
 * 
 * @param msg The message to fill.
 * @param str The string that will be copied to message. Discards the terminating null if it exists.
 * @param slen Length of the string that will be copied.
 */
void fill_msg_data(Message *msg, const char *str, size_t slen) {
	msg->data_count = (int)slen;
	if (slen > 0 && str[slen - 1] == '\0') {
		msg->data_count = (int)slen - 1;
	}

	memcpy(msg->data, str, (size_t)msg->data_count);
}

/**
 * @brief Puts a message in the message queue to all players in the current lobby.
 * 
 * @param l The lobby that the message will be sent to.
 * @param str The string that will be sent. If str has a terminating null
 * character it will be discarded.
 * @param slen The length of the string that will be sent.
 * @return -1 if the message failed to be put in queue. 0 if the message was
 * successfully put in queue.
 */
// static int broadcast_all(Lobby *l, const char *str, size_t slen) {
// 	Message msg;
	
// 	fill_msg_data(&msg, str, slen);

// 	msg.to_count = l->players_count;
// 	// TOOD: Split in to 2 messages if players_count > MSG_MAX_RECIPIENTS.
// 	for (int i = 0; i < l->players_count && i < MSG_MAX_RECIPIENTS; i++) {
// 		msg.to[i] = l->players[i].fd;
// 	}

// 	return msgq_put(l->msgq, &msg);
// }

/**
 * @brief Puts a message in the message queue to all players in the current
 * lobby except for the given player.
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
 * @brief Puts a message in the message queue only to the given player fd.
 * 
 * @param l The lobby the player is in.
 * @param str The string that will be sent. If str has a terminating null
 * character it will be discarded.
 * @param slen The length of the string that will be sent.
 * @param fd The file descriptor of the player.
 */
// static int broadcast(Lobby  *l, const char *str, size_t slen, int fd) {
// 	Message msg;

// 	fill_msg_data(&msg, str, slen);

// 	msg.to_count = 1;
// 	msg.to[0] = fd;

// 	return msgq_put(l->msgq, &msg);
// }

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
bool lobby_full(Lobby *l) {
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

int lobby_leave(Lobby *l, int playerfd) {
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

	return 0;
}

int lobby_play_move(Lobby *l, int playerfd, const char *row_str, const char *col_str) {
	bool is_overflow = false;

	size_t row = (size_t)strtol(row_str, NULL, 10);
	is_overflow = errno == ERANGE;

	size_t col = (size_t)strtol(col_str, NULL, 10);
	is_overflow = is_overflow || errno == ERANGE;

	if (is_overflow || row > l->board->rows || col > l->board->cols) {
		return -1;
	}

	Player *found = NULL;
	for (int i = 0; i < l->players_count; i++) {
		if (l->players[i].fd == playerfd) {
			found = &l->players[i];
		}
	}

	if (!found) {
		return -1;
	}

	char buf[128];
	int buf_size = snprintf(buf, 128, "MOVE %s %s\r\n", row_str, col_str);
	if (buf_size == -1) {
		return -1;
	}

	fprintf(stderr, "%s  %d\n", buf, buf_size);

	board_set(l->board, found->team, (BoardPos){ .row = row, .col = col });

	broadcast_from(l, buf, (size_t)buf_size, playerfd);

	return 0;
}
