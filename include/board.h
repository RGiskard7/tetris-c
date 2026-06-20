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
 * @brief Locks a set of four blocks onto the board with the given type.
 *
 * @param board Pointer to the BOARD.
 * @param blocks Array of 4 (x, y) pairs in board coordinates.
 * @param type Piece type (0-6) stored with each locked cell, selecting
 *             which Game Boy tile is drawn for it.
 * @return OK on success, ERROR if board is NULL.
 */
STATUS board_lock_piece(BOARD *board, int blocks[4][2], int type);

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
 * @brief Renders the board: Game Boy well background and locked tiles.
 *
 * @param board Pointer to the BOARD.
 * @param playfield Playfields sheet for the well background, or NULL.
 * @param tiles Tiles sheet for the locked blocks, or NULL.
 * @return OK on success, ERROR if board is NULL.
 */
STATUS board_print(BOARD *board, ALLEGRO_BITMAP *playfield,
    ALLEGRO_BITMAP *tiles);

#endif /* BOARD_H */
