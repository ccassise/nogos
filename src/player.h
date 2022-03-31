#ifndef PLAYER_H_
#define PLAYER_H_

#include <stdbool.h>

#define PLAYER_NAME_SIZE 32

typedef struct Player {
	char name[PLAYER_NAME_SIZE];
	char team;
	bool is_login;

	int fd; // accept(2)'d file descriptor.

	// Expected to be a queue where each element is the size of a Message. If
	// the default 'player_write' is not used then this can be set to NULL.
	struct Queue *msgq; 

	// Register custom read/write functions.
	long (*read)(const struct Player*, void*, size_t);
	long (*write)(const struct Player*, const void*, size_t);
} Player;

/**
 * @brief Sends a message across a socket to the given player. If msgq is NULL
 * then this function acts the same as send(2). If msgq is not NULL then the
 * messages will be added to the queue instead of being sent immediately.
 * 
 * @param p The player to send the message to.
 * @param buf The message to send.
 * @param size The size of buf.
 * @return long The amount of bytes sent. Send send(2) for comprehensive
 * description. If msgq is not NULL then this function will return -1 if size is
 * too large to fit inside of a Message.
 */
long player_write(const struct Player *p, const void *buf, size_t size);

/**
 * @brief Reads data that was sent across a socket from the given player. This
 * function can be thought of as the same as recv(2).
 * 
 * @param p The player to recieve data from.
 * @param buf The buffer to insert the data.
 * @param size The size of the buffer.
 * @return long The amount of bytes recieved. See recv(2) for comprehensive description.
 */
long player_read(const struct Player *p, void *buf, size_t size);

#endif
