#ifndef PROTOCOL_H_
#define PROTOCOL_H_

#include <stdio.h>

#include "board.h"

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
 * @brief Parses the given buffer and returns its representation.
 * 
 * @param stream* The stream to read from.
 * @return Protocol
 */
Protocol pro_parse(FILE *stream);

#endif
