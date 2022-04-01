#ifndef PROTOCOL_H_
#define PROTOCOL_H_

#define PRO_ARG_SIZE 32

typedef enum {
	PRO_ERROR,
	PRO_LOGIN,
	PRO_LOGOUT,
	PRO_JOIN,
	PRO_LEAVE,
	PRO_MOVE,
} ProtocolMessage;

typedef struct Protocol {
	char arg1[PRO_ARG_SIZE];
	char arg2[PRO_ARG_SIZE];

	ProtocolMessage type;
} Protocol;

/**
 * @brief Parses the given buffer and returns the parsed command and arguments
 * if any.
 * 
 * @param buf The buffer to parse.
 * @param buf_len The amount of bytes in the buffer.
 * @return Protocol
 */

Protocol pro_parse(const char *buf, size_t buf_len);

#endif
