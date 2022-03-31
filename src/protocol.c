#include <ctype.h>
#include <stdbool.h>
#include <string.h>

#include "protocol.h"

#define COMMAND_SIZE 16

/**
 * @brief Match the next characters in the stream exactly to the provided string.
 * When finished the stream will be at the first character that was not in the
 * given string.
 * 
 * @param stream The stream to read from.
 * @param str The string that will be matched. Assumed to be null terminated.
 * @return true The next characters in stream match exactly those given in string.
 * @return false The next characters in stream do not match those given in string.
 */
static bool match(FILE *stream, const char *str) {
	int c;
	while ((c = getc(stream)) != EOF && *str && *str == c) {
		str++;
	}

	return *str == '\0';
}

/**
 * @brief Skips all white space. Also skips null character ('\0').
 * 
 * @param stream The stream to read from.
 * @return int The last read space. If there were no spaces then no characters
 * were consumed.
 */
static int skipspace(FILE *stream) {
	int c;
	while ((c = getc(stream)) != EOF) {
		if (!isspace(c) && c != '\0') {
			break;
		}
	}

	if (c != EOF) {
		c = ungetc(c, stream);
	}

	return c;
}

/**
 * @brief Parses a single protocol argument.
 * 
 * @param stream The stream to read from.
 * @param arg The buffer to write the argument to.
 * @param arg_size The size of the buffer.
 * @param ctype The function that is used to check if character is valid for the argument.
 * @return int The last consumed character in stream.
 */
static int parse_arg(FILE *stream, char *arg, int arg_size, int (ctype)(int)) {
	int ch;
	int len = 0;
	while ((ch = getc(stream)) != EOF && len < arg_size) {
		if (!ctype(ch)) {
			break;
		}

		arg[len++] = (char)ch;
		arg[len] = '\0';
	}

	return ch;
}

Protocol pro_parse(FILE *stream) {
	Protocol result = { .type = PRO_ERROR, .arg1 = { '\0' }, .arg2 = { '\0' } };

	int c;
	c = skipspace(stream);

	if (c == 'J') {
		if (match(stream, "JOIN")) {
			result.type = PRO_JOIN;
		}
	}

	if (c == 'L') {
		char command[COMMAND_SIZE];
		c = parse_arg(stream, command, COMMAND_SIZE - 1, isalpha);
		if (strcmp(command, "LEAVE") == 0) {
			result.type = PRO_LEAVE;
		} else if (strcmp(command, "LOGOUT") == 0) {
			result.type = PRO_LOGOUT;
		} else if (strcmp(command, "LOGIN") == 0) {
			c = skipspace(stream);
			c = parse_arg(stream, result.arg1, PRO_ARG_SIZE - 1, isalnum);

			if (result.arg1[0] != '\0') {
				result.type = PRO_LOGIN;
			}
		}
	}

	if (c == 'M') {
		if (match(stream, "MOVE")) {
			c = skipspace(stream);
			c = parse_arg(stream, result.arg1, PRO_ARG_SIZE - 1, isdigit);
			c = skipspace(stream);
			c = parse_arg(stream, result.arg2, PRO_ARG_SIZE - 1, isdigit);

			if (result.arg1[0] != '\0' && result.arg2[0] != '\0') {
				result.type = PRO_MOVE;
			}
		}
	}

	c = skipspace(stream);
	if (c != EOF) {
		result.type = PRO_ERROR;
	}

	return result;
}
