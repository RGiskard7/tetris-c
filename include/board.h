/**
 * @file board.h
 * @brief Declaration of the Board structure and its associated functions.
 *
 * The board holds the 10x22 grid of locked cells and handles
 * collision detection, line clearing and rendering.
 *
 * Author: RGiskard7
 * Date: 20/06/2026
 */

#ifndef BOARD_H
#define BOARD_H

#include <allegro5/allegro.h>
#include "types.h"

typedef struct _board BOARD;

/**
 * @brief Creates a new empty board.
 *
 * @return Pointer to the created BOARD or NULL on allocation failure.
 */
BOARD *board_create(void);

/**
 * @brief Destroys the board and frees allocated memory.
 *
 * @param board Pointer to the BOARD to destroy.
 */
void board_destroy(BOARD *board);

/**
 * @brief Resets every cell on the board to empty.
 *
 * @param board Pointer to the BOARD.
 * @return OK on success, ERROR if board is NULL.
 */
STATUS board_clear(BOARD *board);

/**
 * @brief Checks whether a set of four blocks collides with the board
 *        boundaries or locked cells.
 *
 * @param board Pointer to the BOARD.
 * @param blocks Array of 4 (x, y) pairs in board coordinates.
 * @return true if any block is out of bounds or overlaps a filled cell.
 */
bool board_check_collision(BOARD *board, int blocks[4][2]);

/**
 * @brief Locks a set of four blocks onto the board with the given colour.
 *
 * @param board Pointer to the BOARD.
 * @param blocks Array of 4 (x, y) pairs in board coordinates.
 * @param color Colour to store with each locked cell.
 * @return OK on success, ERROR if board is NULL.
 */
STATUS board_lock_piece(BOARD *board, int blocks[4][2], ALLEGRO_COLOR color);

/**
 * @brief Clears every completed row and returns how many were cleared.
 *
 * @param board Pointer to the BOARD.
 * @return Number of rows cleared, or 0.
 */
int board_clear_lines(BOARD *board);

/**
 * @brief Checks whether the board is in a game-over state (locked cells
 *        reach into the hidden top rows).
 *
 * @param board Pointer to the BOARD.
 * @return true if game over, false otherwise.
 */
bool board_is_game_over(BOARD *board);

/**
 * @brief Retrieves the colour stored in a given cell.
 *
 * @param board Pointer to the BOARD.
 * @param row Row index (0 = top, hidden).
 * @param col Column index.
 * @return Colour of the cell, or black (0,0,0) if empty.
 */
ALLEGRO_COLOR board_get_cell_color(BOARD *board, int row, int col);

/**
 * @brief Renders the board (background, grid, border and locked cells).
 *
 * @param board Pointer to the BOARD.
 * @return OK on success, ERROR if board is NULL.
 */
STATUS board_print(BOARD *board);

#endif /* BOARD_H */
