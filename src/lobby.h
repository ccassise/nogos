#ifndef LOBBY_H_
#define LOBBY_H_

#include "board.h"
#include "message_queue.h"

#define LOBBY_MAX_PLAYERS 2

typedef struct {
	int fd; // Player's file descriptor.
	char team;
} Player;

/**
 * @brief Implements a lobby where a single game of Atari Go can be played. The
 * game will start when there are 2 players in the lobby and will end when a
 * player has no liberties. A liberty is when a player's piece or adjacent
 * pieces from the same player have no more adjacent spaces to grow in to.
 * 
 */
typedef struct {
	int players_count; // Current number of players in the lobby.
	Player players[LOBBY_MAX_PLAYERS]; // List of all players.
	struct State *state; // Keeps track of the game state.

	Board *board; // The board that the game will be played on.
	MessageQueue *msgq; // Used to be able to send messages between players.
} Lobby;

/**
 * @brief Create and initializes lobby struct. The passed in MessageQueue should
 * outlive the lobby.
 * 
 * @param rows The number of rows the game board should have.
 * @param cols The number of cols the game board should have.
 * @return Lobby The intialized lobby struct.
 */
Lobby *lobby_create(MessageQueue *msgq, size_t rows, size_t cols);

/**
 * @brief Free the space allocated by create.
 * 
 * @param l The Lobby instance to free.
 */
void lobby_destroy(Lobby *l);

/**
 * @brief Inserts a player into the lobby.
 * 
 * @param l The lobby instance to join.
 * @param playerfd The file descriptor of the player.
 * @return int -1 if the player is unable to join the lobby. 0 if joining the lobby was successful.
 */
int lobby_join(Lobby *l, int playerfd);

/**
 * @brief Removes a player from the lobby.
 * 
 * @param l The lobby instance to leave.
 * @param playerfd The file descriptor of the player.
 */
void lobby_leave(Lobby *l, int playerfd);

/**
 * @brief Places a piece of the given player at the given coordinates. Advances
 * the board state 1 turn.
 * 
 * @param l The lobby instance the move will be played on.
 * @param playerfd The file descriptor of the player.
 * @param row_str String representation of an integer that will be which row to place the piece.
 * @param row_str String representation of an integer that will be which col to place the piece.
 * @return int -1 if the move was unable to be played. 0 if the move was played successfully.
 */
int lobby_play_move(Lobby *l, int playerfd, const char *row_str, const char *col_str);

#endif
