#include <ctype.h>
#include <stdbool.h>

#include "protocol.h"

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
bool match(FILE *stream, const char *str) {
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
int skipspace(FILE *stream) {
	int c;
	while ((c = getc(stream)) != EOF) {
		if (!isspace(c)) {
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
 */
static void parse_arg(FILE *stream, char *arg, int arg_size, int (ctype)(int)) {
	int ch;
	int count = 0;
	while ((ch = getc(stream)) != EOF && count < arg_size) {
		if (!ctype(ch)) {
			break;
		}

		arg[count++] = (char)ch;
		arg[count] = '\0';
	}
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
		if (match(stream, "LEAVE")) {
			result.type = PRO_LEAVE;
		}
	}

	if (c == 'M') {
		if (match(stream, "MOVE")) {
			c = skipspace(stream);

			parse_arg(stream, result.arg1, PRO_ARG_SIZE - 1, isdigit);

			c = skipspace(stream);

			parse_arg(stream, result.arg2, PRO_ARG_SIZE - 1, isdigit);

			if (c != EOF && result.arg1[0] != '\0' && result.arg2[0] != '\0') {
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
