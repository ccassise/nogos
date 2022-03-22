#ifndef LOBBY_H_
#define LOBBY_H_

#include "board.h"
#include "message_queue.h"

#define LOBBY_MAX_PLAYERS 2

typedef struct {
	int fd; // Player's file descriptor.
	char team;
} Player;

typedef struct {
	int players_count;
	Player players[LOBBY_MAX_PLAYERS];

	Board *board;
	MessageQueue *msgq;
} Lobby;

/**
 * @brief Create and initializes lobby struct. The passed in MessageQueue should
 * outlive the lobby.
 * 
 * @return Lobby The intialized lobby struct.
 */
Lobby *lobby_create(MessageQueue *msgq);

/**
 * @brief Free the space allocated by create.
 * 
 * @param l The Lobby instance to free.
 */
void lobby_destroy(Lobby *l);

int lobby_join(Lobby *l, int playerfd);

int lobby_leave(Lobby *l, int playerfd);

int lobby_play_move(Lobby *l, int playerfd, const char *row_str, const char *col_str);

#endif
