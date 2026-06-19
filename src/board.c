/**
 * @file board.c
 * @brief Implementation of the Board structure and associated functions.
 *
 * The board holds a 10x22 grid (2 hidden rows + 20 visible). Each cell
 * tracks whether it is filled and its colour. Handles collision checks,
 * line clearing, and rendering.
 *
 * Author: RGiskard7
 * Date: 20/06/2026
 */

#include "board.h"
#include "config.h"

#include <allegro5/allegro_primitives.h>
#include <stdlib.h>
#include <string.h>

struct _board {
    ALLEGRO_COLOR cells[BOARD_ROWS][BOARD_COLS];
    bool filled[BOARD_ROWS][BOARD_COLS];
};

// =========================================================================
// Functions: Board Creation and Destruction
// =========================================================================

/**
 * @brief Creates an empty board with all cells cleared.
 *
 * @return Pointer to the created board or NULL if allocation fails.
 */
BOARD *board_create(void) {
    BOARD *new_board = NULL;

    new_board = (BOARD *) malloc(sizeof (BOARD));
    if (!new_board) {
        return NULL;
    }

    board_clear(new_board);

    return new_board;
}

/**
 * @brief Destroys the board and frees its memory.
 *
 * @param board Pointer to the board to destroy.
 */
void board_destroy(BOARD *board) {
    if (board != NULL) {
        free(board);
    }
}

/**
 * @brief Resets every cell on the board to empty.
 *
 * @param board Pointer to the board.
 * @return OK on success, ERROR if board is NULL.
 */
STATUS board_clear(BOARD *board) {
    int r = 0;
    int c = 0;

    if (!board) {
        return ERROR;
    }

    for (r = 0; r < BOARD_ROWS; r++) {
        for (c = 0; c < BOARD_COLS; c++) {
            board->cells[r][c] = al_map_rgba(0, 0, 0, 0);
            board->filled[r][c] = false;
        }
    }

    return OK;
}

// =========================================================================
// Functions: Board State
// =========================================================================

/**
 * @brief Checks whether the cell at the given coordinates is occupied.
 *
 * @param board Pointer to the board.
 * @param row Row index.
 * @param col Column index.
 * @return true if filled or out of bounds.
 */
static bool board_is_filled(BOARD *board, int row, int col) {
    if (col < 0 || col >= BOARD_COLS || row < 0 || row >= BOARD_ROWS) {
        return true;
    }

    return board->filled[row][col];
}

/**
 * @brief Checks whether a set of four blocks collides with boundaries
 *        or locked cells.
 *
 * @param board Pointer to the board.
 * @param blocks Array of 4 (x, y) pairs in board coordinates.
 * @return true if any block is out of bounds or overlaps a filled cell.
 */
bool board_check_collision(BOARD *board, int blocks[4][2]) {
    int i = 0;

    if (!board) {
        return true;
    }

    for (i = 0; i < 4; i++) {
        if (board_is_filled(board, blocks[i][1], blocks[i][0])) {
            return true;
        }
    }

    return false;
}

/**
 * @brief Locks a set of four blocks onto the board.
 *
 * @param board Pointer to the board.
 * @param blocks Array of 4 (x, y) pairs in board coordinates.
 * @param color Colour to store.
 * @return OK on success, ERROR if board is NULL.
 */
STATUS board_lock_piece(BOARD *board, int blocks[4][2], ALLEGRO_COLOR color) {
    int i = 0;

    if (!board) {
        return ERROR;
    }

    for (i = 0; i < 4; i++) {
        int x = blocks[i][0];
        int y = blocks[i][1];

        if (x >= 0 && x < BOARD_COLS && y >= 0 && y < BOARD_ROWS) {
            board->cells[y][x] = color;
            board->filled[y][x] = true;
        }
    }

    return OK;
}

/**
 * @brief Checks whether a row is completely filled.
 *
 * @param board Pointer to the board.
 * @param row Row index.
 * @return true if every cell in the row is filled.
 */
static bool board_is_row_full(BOARD *board, int row) {
    int c = 0;

    for (c = 0; c < BOARD_COLS; c++) {
        if (!board->filled[row][c]) {
            return false;
        }
    }

    return true;
}

/**
 * @brief Shifts all rows above from_row down by one row.
 *
 * @param board Pointer to the board.
 * @param from_row Row index that was cleared.
 */
static void board_collapse_rows(BOARD *board, int from_row) {
    int r = 0;
    int c = 0;

    for (r = from_row; r > 0; r--) {
        for (c = 0; c < BOARD_COLS; c++) {
            board->cells[r][c] = board->cells[r - 1][c];
            board->filled[r][c] = board->filled[r - 1][c];
        }
    }

    for (c = 0; c < BOARD_COLS; c++) {
        board->cells[0][c] = al_map_rgba(0, 0, 0, 0);
        board->filled[0][c] = false;
    }
}

/**
 * @brief Clears every completed row and collapses the board.
 *
 * @param board Pointer to the board.
 * @return Number of rows cleared.
 */
int board_clear_lines(BOARD *board) {
    int r = 0;
    int cleared = 0;

    if (!board) {
        return 0;
    }

    r = BOARD_ROWS - 1;
    while (r >= 0) {
        if (board_is_row_full(board, r)) {
            board_collapse_rows(board, r);
            cleared++;
        } else {
            r--;
        }
    }

    return cleared;
}

/**
 * @brief Checks whether any locked cell exists in the hidden rows.
 *
 * @param board Pointer to the board.
 * @return true if a cell in rows 0..BOARD_HIDDEN-1 is filled.
 */
bool board_is_game_over(BOARD *board) {
    int r = 0;
    int c = 0;

    if (!board) {
        return true;
    }

    for (r = 0; r < BOARD_HIDDEN; r++) {
        for (c = 0; c < BOARD_COLS; c++) {
            if (board->filled[r][c]) {
                return true;
            }
        }
    }

    return false;
}

/**
 * @brief Retrieves the colour stored in a given cell.
 *
 * @param board Pointer to the board.
 * @param row Row index.
 * @param col Column index.
 * @return Colour of the cell, or black if empty or out of bounds.
 */
ALLEGRO_COLOR board_get_cell_color(BOARD *board, int row, int col) {
    if (!board || col < 0 || col >= BOARD_COLS
        || row < 0 || row >= BOARD_ROWS) {
        return al_map_rgba(0, 0, 0, 0);
    }

    return board->cells[row][col];
}

// =========================================================================
// Functions: Rendering
// =========================================================================

/**
 * @brief Renders the board: background, grid lines, border, and locked cells.
 *
 * @param board Pointer to the board.
 * @return OK on success, ERROR if board is NULL.
 */
STATUS board_print(BOARD *board) {
    int r = 0;
    int c = 0;
    float x0 = (float) BOARD_X0;
    float y0 = (float) BOARD_Y0;
    float cs = (float) CELL_SIZE;
    float bx = 0.0f;
    float by = 0.0f;

    if (!board) {
        return ERROR;
    }

    // board filled background
    al_draw_filled_rectangle(x0, y0,
        x0 + BOARD_COLS * cs, y0 + BOARD_VISIBLE * cs, COLOR_BOARD_BG);

    // locked cells (only visible rows)
    for (r = BOARD_HIDDEN; r < BOARD_ROWS; r++) {
        for (c = 0; c < BOARD_COLS; c++) {
            if (board->filled[r][c]) {
                bx = x0 + c * cs;
                by = y0 + (r - BOARD_HIDDEN) * cs;

                al_draw_filled_rectangle(bx, by, bx + cs, by + cs,
                    board->cells[r][c]);
                al_draw_filled_rectangle(bx + 1.0f, by + 1.0f,
                    bx + cs - 2.0f, by + cs - 2.0f,
                    board->cells[r][c]);
            }
        }
    }

    // grid lines
    for (r = 0; r <= BOARD_VISIBLE; r++) {
        al_draw_line(x0, y0 + r * cs,
            x0 + BOARD_COLS * cs, y0 + r * cs, COLOR_GRID, 1.0f);
    }
    for (c = 0; c <= BOARD_COLS; c++) {
        al_draw_line(x0 + c * cs, y0,
            x0 + c * cs, y0 + BOARD_VISIBLE * cs, COLOR_GRID, 1.0f);
    }

    // outer border
    al_draw_rectangle(x0 - BORDER_W, y0 - BORDER_W,
        x0 + BOARD_COLS * cs + BORDER_W,
        y0 + BOARD_VISIBLE * cs + BORDER_W, COLOR_BORDER, BORDER_W);

    return OK;
}
