#include <stdlib.h>
#include <string.h>

#include "task.h"

#include "board.h"

static void test_board_create(void) {
	const size_t rows = 3;
	const size_t cols = 5;

	Board *b = board_create(rows, cols);

	for (size_t i = 0; i < rows * cols; i++) {
		assert(b->board[i] == BOARD_EMPTY_SPACE);
	}

	board_destroy(b);
}

static void test_board_set(void) {
	Board *b = board_create(3, 5);

	board_set(b, 'O', (BoardPos){ .row = 0, .col = 3 });
	board_set(b, 'X', (BoardPos){ .row = 1, .col = 2 });
	board_set(b, 'O', (BoardPos){ .row = 2, .col = 4 });

	char *actual = board_str(b);
	const char *expect = (
		". . . O .\n"
		". . X . .\n"
		". . . . O"
	);

	ASSERT(strcmp(actual, expect) == 0);

	free(actual);
	board_destroy(b);
}

static void test_board_get(void) {
	Board *b = board_create(3, 5);

	board_set(b, 'O', (BoardPos){ .row = 0, .col = 3 });
	board_set(b, 'X', (BoardPos){ .row = 1, .col = 2 });
	board_set(b, 'O', (BoardPos){ .row = 2, .col = 4 });

	ASSERT(board_get(b, (BoardPos){ .row = 0, .col = 0 }) == BOARD_EMPTY_SPACE);
	ASSERT(board_get(b, (BoardPos){ .row = 0, .col = 3 }) == 'O');
	ASSERT(board_get(b, (BoardPos){ .row = 1, .col = 2 }) == 'X');
	ASSERT(board_get(b, (BoardPos){ .row = 2, .col = 4 }) == 'O');

	board_destroy(b);
}

int main(void) {
	test_board_create();
	test_board_set();
	test_board_get();
}
