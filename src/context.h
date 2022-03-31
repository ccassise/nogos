#ifndef CONTEXT_H_
#define CONTEXT_H_

#include <stddef.h>

typedef struct Context {
	struct Lobby *l;
	struct Queue *msgq; // Contains messages that need to be sent.
	struct Queue *closeq; // Contains file descriptors that need to be closed.

	struct Player *players;
	size_t players_len;
	size_t players_size;

	struct pollfd *pfds;
	size_t pfds_len;
	size_t pfds_size;
} Context;

/**
 * @brief Creates and initializes a context instance.
 * 
 * @return Context* The created instance. NULL if an error occurred.
 */
Context *ctx_create(void);

/**
 * @brief Free any allocated memory of given context instance.
 * 
 * @param ctx The instance to free.
 */
void ctx_destory(Context *ctx);

/**
 * @brief Adds the given player to context's list of players.
 * 
 * @param ctx The context instance to add to.
 * @param player The player to be added.
 * @return int -1 on error. 0 otherwise.
 */
int ctx_add_player(Context *ctx, const struct Player *player);

/**
 * @brief Get the player that is associated to the given file descriptor.
 * 
 * @param ctx The context isntance to search.
 * @param fd The file descriptor  to match.
 * @return Player* The player that matches the associated file descriptor. Returns NULL if the player is not found. 
 */
struct Player *ctx_get_player(Context *ctx, int fd);

/**
 * @brief Remove the given player from context's list of players.
 * 
 * @param ctx The context instance to be removed from.
 * @param player The player to be removed.
 */
void ctx_remove_player(Context *ctx, int fd);

#endif
