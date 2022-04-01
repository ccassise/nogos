#include <ctype.h>
#include <stdbool.h>
#include <string.h>

#include "protocol.h"

#define COMMAND_SIZE 16

typedef struct Parser {
	const char *buf;
	size_t buf_size;
	size_t cur_pos;
} Parser;

/**
 * @brief Consume a character from a buffer and advance the buffer position by one.
 * 
 * @return int The character that was consumed or -1 if reached end of buffer.
 */
static int pgetc(Parser *p) {
	if (p->cur_pos == p->buf_size) {
		return -1;
	}
	return p->buf[p->cur_pos++];
}

/**
 * @brief Unget a character in the buffer. Calling this operation before pgetc
 * will return the first character in the buffer and not decrement the current
 * position in the buffer.
 * 
 * @return int The character that was put back.
 */
static int pungetc(Parser *p) {
	if (p->cur_pos != 0) {
		p->cur_pos--;
		return p->buf[p->cur_pos];
	}

	return p->buf[p->cur_pos];
}

/**
 * @brief Match the next characters in the buffer exactly to the provided string.
 * When finished the buffer will be at the first character that was not in the
 * given string.
 * 
 * @param buf The buffer to read from.
 * @param str The string that will be matched. Assumed to be null terminated.
 * @return true The next characters in buffer match exactly those given in string.
 * @return false The next characters in buffer do not match those given in string.
 */
static bool match(Parser *p, const char *str) {
	int c;
	while ((c = pgetc(p)) != -1 && *str && *str == c) {
		str++;
	}

	return *str == '\0';
}

/**
 * @brief Skips all white space. Also skips null character ('\0').
 * 
 * @param buf The buffer to read from.
 * @return int The last read space. If there were no spaces then no characters
 * were consumed.
 */
static int skipspace(Parser *p) {
	int c;
	while ((c = pgetc(p)) != -1) {
		if (!isspace(c) && c != '\0') {
			break;
		}
	}

	if (c != -1) {
		c = pungetc(p);
	}

	return c;
}

/**
 * @brief Parses a single protocol argument.
 * 
 * @param buf The buffer to read from.
 * @param arg The buffer to write the argument to.
 * @param arg_size The size of the buffer.
 * @param ctype The function that is used to check if character is valid for the argument.
 * @return int The last consumed character in buf.
 */
static int parse_arg(Parser *p, char *arg, int arg_size, int (ctype)(int)) {
	int ch;
	int len = 0;
	while ((ch = pgetc(p)) != -1 && len < arg_size) {
		if (!ctype(ch)) {
			break;
		}

		arg[len++] = (char)ch;
		arg[len] = '\0';
	}

	return ch;
}

Protocol pro_parse(const char *buf, size_t buf_size) {
	Protocol result = { .type = PRO_ERROR, .arg1 = { '\0' }, .arg2 = { '\0' } };
	Parser parser = { .buf = buf, .buf_size = buf_size, .cur_pos = 0 };

	int c;
	c = skipspace(&parser);

	if (c == 'J') {
		if (match(&parser, "JOIN")) {
			result.type = PRO_JOIN;
		}
	}

	if (c == 'L') {
		char command[COMMAND_SIZE];
		c = parse_arg(&parser, command, COMMAND_SIZE - 1, isalpha);
		if (strcmp(command, "LEAVE") == 0) {
			result.type = PRO_LEAVE;
		} else if (strcmp(command, "LOGOUT") == 0) {
			result.type = PRO_LOGOUT;
		} else if (strcmp(command, "LOGIN") == 0) {
			c = skipspace(&parser);
			c = parse_arg(&parser, result.arg1, PRO_ARG_SIZE - 1, isalnum);

			if (result.arg1[0] != '\0') {
				result.type = PRO_LOGIN;
			}
		}
	}

	if (c == 'M') {
		if (match(&parser, "MOVE")) {
			c = skipspace(&parser);
			c = parse_arg(&parser, result.arg1, PRO_ARG_SIZE - 1, isdigit);
			c = skipspace(&parser);
			c = parse_arg(&parser, result.arg2, PRO_ARG_SIZE - 1, isdigit);

			if (result.arg1[0] != '\0' && result.arg2[0] != '\0') {
				result.type = PRO_MOVE;
			}
		}
	}

	c = skipspace(&parser);
	if (c != -1) {
		result.type = PRO_ERROR;
	}

	return result;
}
