/**
 * @file piece.h
 * @brief Declaration of the Piece structure and its associated functions.
 *
 * A piece is one of the seven classic tetrominoes (I, O, T, S, Z, J, L).
 * It tracks its type, rotation state, and position on the board.
 * Shapes are defined per rotation state as four (x, y) offsets relative
 * to the piece origin.
 *
 * Author: RGiskard7
 * Date: 20/06/2026
 */

#ifndef PIECE_H
#define PIECE_H

#include <allegro5/allegro.h>
#include "types.h"

typedef struct _piece PIECE;

/**
 * @brief Creates a new piece of a random type at the spawn position.
 *
 * @return Pointer to the created PIECE or NULL on allocation failure.
 */
PIECE *piece_create(void);

/**
 * @brief Destroys the piece and frees allocated memory.
 *
 * @param piece Pointer to the PIECE to destroy.
 */
void piece_destroy(PIECE *piece);

/**
 * @brief Randomises the piece type, resets rotation, and places it at
 *        the spawn position.
 *
 * @param piece Pointer to the PIECE.
 * @return OK on success, ERROR if piece is NULL.
 */
STATUS piece_reset(PIECE *piece);

// Getters
int piece_get_type(PIECE *piece);
int piece_get_rotation(PIECE *piece);
int piece_get_x(PIECE *piece);
int piece_get_y(PIECE *piece);

// Setters
STATUS piece_set_type(PIECE *piece, int type);
STATUS piece_set_rotation(PIECE *piece, int rotation);
STATUS piece_set_x(PIECE *piece, int x);
STATUS piece_set_y(PIECE *piece, int y);

/**
 * @brief Fills an array with the four block coordinates of the piece
 *        in its current type, rotation and position.
 *
 * @param piece Pointer to the PIECE.
 * @param blocks Output array of 4 (x, y) pairs in board coordinates.
 */
void piece_get_blocks(PIECE *piece, int blocks[4][2]);

/**
 * @brief Fills an array with the four block coordinates the piece would
 *        have with the given parameters, without modifying the piece.
 *
 * @param type Piece type (0-6).
 * @param rotation Rotation state (0-3).
 * @param x Column origin on the board.
 * @param y Row origin on the board.
 * @param blocks Output array of 4 (x, y) pairs.
 */
void piece_get_blocks_at(int type, int rotation, int x, int y,
    int blocks[4][2]);

/**
 * @brief Moves the piece by the given delta.
 *
 * @param piece Pointer to the PIECE.
 * @param dx Column delta.
 * @param dy Row delta.
 * @return OK on success, ERROR if piece is NULL.
 */
STATUS piece_move(PIECE *piece, int dx, int dy);

/**
 * @brief Rotates the piece clockwise (adds 1 to rotation, wraps at 4).
 *
 * @param piece Pointer to the PIECE.
 * @return OK on success, ERROR if piece is NULL.
 */
STATUS piece_rotate_cw(PIECE *piece);

/**
 * @brief Gets the colour associated with a given piece type.
 *
 * @param type Piece type (0-6).
 * @return Allegro colour for that tetromino.
 */
ALLEGRO_COLOR piece_get_color(int type);

/**
 * @brief Draws a single block tile scaled from the Game Boy sheet.
 *
 * Cuts the 8x8 source tile for the given piece type out of the tiles
 * sheet and scales it to the destination square. If tiles is NULL it
 * falls back to a solid colour rectangle so the game still runs.
 *
 * @param tiles Tiles sprite sheet, or NULL for the colour fallback.
 * @param type Piece type (0-6) selecting which tile to draw.
 * @param x Destination top-left x in pixels.
 * @param y Destination top-left y in pixels.
 * @param size Destination square side in pixels.
 * @param alpha Opacity in [0, 1] (used for the ghost piece).
 */
void piece_draw_tile(ALLEGRO_BITMAP *tiles, int type, float x, float y,
    float size, float alpha);

/**
 * @brief Tells whether a block list contains the cell (x, y).
 *
 * @param blocks Array of 4 (x, y) pairs.
 * @param x Column to look for.
 * @param y Row to look for.
 * @return true if some block is at (x, y).
 */
bool piece_blocks_has(int blocks[4][2], int x, int y);

/**
 * @brief Draws one I-beam cell, joined to its I-piece neighbours.
 *
 * The I piece is one bar, not four blocks: the neighbour flags pick the
 * end caps and the orientation so the cells join seamlessly.
 *
 * @param tiles Tiles sheet, or NULL for the colour fallback.
 * @param left True if the left neighbour is also an I block.
 * @param right True if the right neighbour is also an I block.
 * @param up True if the upper neighbour is also an I block.
 * @param down True if the lower neighbour is also an I block.
 * @param x Destination x.
 * @param y Destination y.
 * @param size Destination side.
 * @param alpha Opacity (1 = opaque).
 */
void piece_draw_ibeam(ALLEGRO_BITMAP *tiles, bool left, bool right,
    bool up, bool down, float x, float y, float size, float alpha);

/**
 * @brief Renders the piece on the board using the Game Boy block tiles.
 *
 * @param piece Pointer to the PIECE.
 * @param tiles Tiles sprite sheet, or NULL for the colour fallback.
 * @param ghost If true draws translucent (ghost piece).
 * @return OK on success, ERROR if piece is NULL.
 */
STATUS piece_print(PIECE *piece, ALLEGRO_BITMAP *tiles, bool ghost);

#endif /* PIECE_H */
