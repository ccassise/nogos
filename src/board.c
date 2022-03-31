#include <stdlib.h>
#include <string.h>

#include "board.h"

/**
 * @brief Gets the 1D index from a position.
 * 
 * @param b The board instance.
 * @param pos The position to convert.
 * @return size_t The array index of the given position.
 */
static size_t pos_index(size_t cols, BoardPos pos) {
	return pos.row * cols + pos.col; 
}

Board *board_create(size_t rows, size_t cols) {
	Board *b = malloc(sizeof *b);
	b->rows = rows;
	b->cols = cols;

	b->board = calloc(b->rows * b->cols, sizeof *b->board);

	return b;
}

void board_free(Board *b) {
	free(b->board);
	free(b);
}

char board_get(Board *b, BoardPos pos) {
	return b->board[pos_index(b->cols, pos)];
}

void board_set(Board *b, char piece, BoardPos pos) {
	b->board[pos_index(b->cols, pos)] = piece;
}

char *board_str(Board *b) {
	// Allocate enough space so that each position on the board can be separated
	// by a space and each line can have a newline. The string will have a
	// terminating null character where the last newline would be.
	const size_t len = (b->rows * b->cols * 2);
	char *result = malloc((sizeof *result) * len);

	for (size_t i = 0, j = 0; i < b->rows * b->cols; i++) {
		if (b->board[i] == BOARD_EMPTY_SPACE) {
			result[j++] = '.';
		} else {
			result[j++] = b->board[i];
		}

		if (i % b->cols == b->cols - 1) {
			result[j++] = '\n';
		} else {
			result[j++] = ' ';
		}
	}

	result[len - 1] = '\0';

	return result;
}
