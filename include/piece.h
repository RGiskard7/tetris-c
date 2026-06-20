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
 * @brief Renders the piece on the board.
 *
 * @param piece Pointer to the PIECE.
 * @return OK on success, ERROR if piece is NULL.
 */
STATUS piece_print(PIECE *piece);

#endif /* PIECE_H */
