#ifndef BOARD_H_
#define BOARD_H_

#include <stddef.h>

#define BOARD_EMPTY_SPACE '\0'

/**
 * @brief Represents a board object. A board is a square where the top left
 * corner is 0,0 (row, then column).
 * 
 */
typedef struct Board {
	size_t rows;
	size_t cols;
	char *board;
} Board;

/**
 * @brief A position on the board.
 * 
 */
typedef struct BoardPos {
	size_t row;
	size_t col;
} BoardPos;

/**
 * @brief Create a board object. 
 * The user must call board_free on the returned Board when done.
 * 
 * @param rows Number of rows the board will have.
 * @param cols Number of columns a board will have.
 * @return * Board The newly created board object.
 */
Board *board_create(size_t rows, size_t cols);

/**
 * @brief Free the memory that was used by board.
 * 
 * @param b The board to free.
 */
void board_free(Board *b);

/**
 * @brief Get the char at the given board position.
 * 
 * @param b The board from which to get the piece.
 * @param pos The board position to get the character from.
 * @return char The character at the given position.
 */
char board_get(Board *b, BoardPos pos);

/**
 * @brief Places a given piece onto the board at given position.
 * 
 * @param Board The board in which to place a piece.
 * @param piece The piece represented as a character to place.
 * @param pos The position where to place the piece.
 */
void board_set(Board *b, char piece, BoardPos pos);

/**
 * @brief Returns a string representation of the board. Allocates memory for the
 * string and can be freed with free.
 * 
 * @param b The board the stringify.
 * @return char* The string representation of a board. Blank spaces will be displayed as a dot.
 */
char *board_str(Board *b);

#endif
