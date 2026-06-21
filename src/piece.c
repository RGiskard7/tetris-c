/**
 * @file piece.c
 * @brief Implementation of the Piece structure and associated functions.
 *
 * Stores the seven classic tetromino shapes, each with four rotation
 * states defined as four (x, y) block offsets. Provides creation,
 * position manipulation, block coordinate calculation and rendering.
 *
 * Author: RGiskard7
 * Date: 20/06/2026
 */

#include "piece.h"
#include "config.h"

#include <allegro5/allegro_primitives.h>
#include <stdlib.h>

/**
 * @brief Block offsets for each piece type, rotation state, and block index.
 *
 * SHAPES[type][rotation][block][axis]
 *   type:     0-I, 1-O, 2-T, 3-S, 4-Z, 5-J, 6-L
 *   rotation: 0-spawn, 1-CW, 2-180, 3-CCW
 *   block:    0..3 (four blocks per piece)
 *   axis:     0=x (column), 1=y (row)
 *
 * Offsets are relative to the piece origin (top-left of the 4x4
 * bounding box) and are applied as (origin_x + offset_x,
 * origin_y + offset_y) to get board coordinates.
 */
static const int SHAPES[PIECE_TYPES][4][4][2] = {
    // I
    {
        {{0,1}, {1,1}, {2,1}, {3,1}},
        {{2,0}, {2,1}, {2,2}, {2,3}},
        {{0,2}, {1,2}, {2,2}, {3,2}},
        {{1,0}, {1,1}, {1,2}, {1,3}}
    },
    // O
    {
        {{1,0}, {2,0}, {1,1}, {2,1}},
        {{1,0}, {2,0}, {1,1}, {2,1}},
        {{1,0}, {2,0}, {1,1}, {2,1}},
        {{1,0}, {2,0}, {1,1}, {2,1}}
    },
    // T
    {
        {{1,0}, {0,1}, {1,1}, {2,1}},
        {{1,0}, {1,1}, {2,1}, {1,2}},
        {{0,1}, {1,1}, {2,1}, {1,2}},
        {{1,0}, {0,1}, {1,1}, {1,2}}
    },
    // S
    {
        {{1,0}, {2,0}, {0,1}, {1,1}},
        {{1,0}, {1,1}, {2,1}, {2,2}},
        {{1,1}, {2,1}, {0,2}, {1,2}},
        {{0,0}, {0,1}, {1,1}, {1,2}}
    },
    // Z
    {
        {{0,0}, {1,0}, {1,1}, {2,1}},
        {{2,0}, {1,1}, {2,1}, {1,2}},
        {{0,1}, {1,1}, {1,2}, {2,2}},
        {{1,0}, {0,1}, {1,1}, {0,2}}
    },
    // J
    {
        {{0,0}, {0,1}, {1,1}, {2,1}},
        {{1,0}, {2,0}, {1,1}, {1,2}},
        {{0,1}, {1,1}, {2,1}, {2,2}},
        {{1,0}, {1,1}, {0,2}, {1,2}}
    },
    // L
    {
        {{2,0}, {0,1}, {1,1}, {2,1}},
        {{1,0}, {1,1}, {1,2}, {2,2}},
        {{0,1}, {1,1}, {2,1}, {0,2}},
        {{0,0}, {1,0}, {1,1}, {1,2}}
    }
};

/**
 * @brief Returns the colour associated with a given piece type.
 *
 * Uses a switch so al_map_rgb is evaluated at runtime, not as a
 * compile-time static initializer (which would fail the const-expr check).
 *
 * @param type Piece type (0-6).
 * @return Allegro colour for that tetromino.
 */
ALLEGRO_COLOR piece_get_color(int type) {
    switch (type) {
        case 0:
            return COLOR_I;
        case 1:
            return COLOR_O;
        case 2:
            return COLOR_T;
        case 3:
            return COLOR_S;
        case 4:
            return COLOR_Z;
        case 5:
            return COLOR_J;
        case 6:
            return COLOR_L;
        default:
            return al_map_rgb(255, 255, 255);
    }
}

struct _piece {
    int type;       ///< Tetromino type (0-6)
    int rotation;   ///< Rotation state (0-3)
    int x;          ///< Column of the piece origin on the board
    int y;          ///< Row of the piece origin on the board
};

// =========================================================================
// Functions: Piece Creation and Destruction
// =========================================================================

/**
 * @brief Creates a piece with a random type at the spawn position.
 *
 * @return Pointer to the created piece or NULL if allocation fails.
 */
PIECE *piece_create(void) {
    PIECE *new_piece = NULL;

    new_piece = (PIECE *) malloc(sizeof (PIECE));
    if (!new_piece) {
        return NULL;
    }

    new_piece->type = rand() % PIECE_TYPES;
    new_piece->rotation = 0;
    new_piece->x = SPAWN_X;
    new_piece->y = SPAWN_Y;

    return new_piece;
}

/**
 * @brief Destroys the piece and frees its memory.
 *
 * @param piece Pointer to the piece to destroy.
 */
void piece_destroy(PIECE *piece) {
    if (piece != NULL) {
        free(piece);
    }
}

/**
 * @brief Randomises the piece type, resets rotation, and moves it to
 *        the spawn position.
 *
 * @param piece Pointer to the piece.
 * @return OK on success, ERROR if piece is NULL.
 */
STATUS piece_reset(PIECE *piece) {
    if (!piece) {
        return ERROR;
    }

    piece->type = rand() % PIECE_TYPES;
    piece->rotation = 0;
    piece->x = SPAWN_X;
    piece->y = SPAWN_Y;

    return OK;
}

// =========================================================================
// Functions: Accessors
// =========================================================================

/**
 * @brief Retrieves the piece type.
 *
 * @param piece Pointer to the piece.
 * @return Type index (0-6), or 0 if piece is NULL.
 */
int piece_get_type(PIECE *piece) {
    if (!piece) {
        return 0;
    }

    return piece->type;
}

/**
 * @brief Retrieves the current rotation state.
 *
 * @param piece Pointer to the piece.
 * @return Rotation (0-3), or 0 if piece is NULL.
 */
int piece_get_rotation(PIECE *piece) {
    if (!piece) {
        return 0;
    }

    return piece->rotation;
}

/**
 * @brief Retrieves the column of the piece origin.
 *
 * @param piece Pointer to the piece.
 * @return Column index, or 0 if piece is NULL.
 */
int piece_get_x(PIECE *piece) {
    if (!piece) {
        return 0;
    }

    return piece->x;
}

/**
 * @brief Retrieves the row of the piece origin.
 *
 * @param piece Pointer to the piece.
 * @return Row index, or 0 if piece is NULL.
 */
int piece_get_y(PIECE *piece) {
    if (!piece) {
        return 0;
    }

    return piece->y;
}

/**
 * @brief Sets the piece type.
 *
 * @param piece Pointer to the piece.
 * @param type Type index (0-6).
 * @return OK on success, ERROR if piece is NULL.
 */
STATUS piece_set_type(PIECE *piece, int type) {
    if (!piece) {
        return ERROR;
    }

    piece->type = type;

    return OK;
}

/**
 * @brief Sets the rotation state.
 *
 * @param piece Pointer to the piece.
 * @param rotation Rotation index (0-3).
 * @return OK on success, ERROR if piece is NULL.
 */
STATUS piece_set_rotation(PIECE *piece, int rotation) {
    if (!piece) {
        return ERROR;
    }

    piece->rotation = rotation % 4;

    return OK;
}

/**
 * @brief Sets the column of the piece origin.
 *
 * @param piece Pointer to the piece.
 * @param x Column index.
 * @return OK on success, ERROR if piece is NULL.
 */
STATUS piece_set_x(PIECE *piece, int x) {
    if (!piece) {
        return ERROR;
    }

    piece->x = x;

    return OK;
}

/**
 * @brief Sets the row of the piece origin.
 *
 * @param piece Pointer to the piece.
 * @param y Row index.
 * @return OK on success, ERROR if piece is NULL.
 */
STATUS piece_set_y(PIECE *piece, int y) {
    if (!piece) {
        return ERROR;
    }

    piece->y = y;

    return OK;
}

// =========================================================================
// Functions: Block Calculation
// =========================================================================

/**
 * @brief Fills an array with the four block coordinates of the piece
 *        in its current state.
 *
 * @param piece Pointer to the piece.
 * @param blocks Output array of 4 (x, y) pairs.
 */
void piece_get_blocks(PIECE *piece, int blocks[4][2]) {
    if (!piece) {
        return;
    }

    piece_get_blocks_at(piece->type, piece->rotation, piece->x, piece->y,
        blocks);
}

/**
 * @brief Fills an array with block coordinates for a given piece state.
 *
 * @param type Piece type (0-6).
 * @param rotation Rotation (0-3).
 * @param x Column origin.
 * @param y Row origin.
 * @param blocks Output array of 4 (x, y) pairs.
 */
void piece_get_blocks_at(int type, int rotation, int x, int y,
    int blocks[4][2]) {
    int i = 0;

    if (type < 0 || type >= PIECE_TYPES) {
        return;
    }

    rotation = rotation % 4;
    if (rotation < 0) {
        rotation += 4;
    }

    for (i = 0; i < 4; i++) {
        blocks[i][0] = x + SHAPES[type][rotation][i][0];
        blocks[i][1] = y + SHAPES[type][rotation][i][1];
    }
}

// =========================================================================
// Functions: Movement and Rotation
// =========================================================================

/**
 * @brief Moves the piece by the given delta.
 *
 * @param piece Pointer to the piece.
 * @param dx Column delta.
 * @param dy Row delta.
 * @return OK on success, ERROR if piece is NULL.
 */
STATUS piece_move(PIECE *piece, int dx, int dy) {
    if (!piece) {
        return ERROR;
    }

    piece->x += dx;
    piece->y += dy;

    return OK;
}

/**
 * @brief Rotates the piece clockwise (adds 1, wraps at 4).
 *
 * @param piece Pointer to the piece.
 * @return OK on success, ERROR if piece is NULL.
 */
STATUS piece_rotate_cw(PIECE *piece) {
    if (!piece) {
        return ERROR;
    }

    piece->rotation = (piece->rotation + 1) % 4;

    return OK;
}

// =========================================================================
// Functions: Rendering
// =========================================================================

/**
 * @brief Draws an 8x8 sheet tile scaled to the cell.
 *
 * @param tiles Tiles sprite sheet.
 * @param sx Source x on the sheet.
 * @param sy Source y on the sheet.
 * @param x Destination x.
 * @param y Destination y.
 * @param size Destination side.
 * @param alpha Opacity (1 = opaque, < 1 for the ghost).
 */
static void piece_blit(ALLEGRO_BITMAP *tiles, int sx, int sy,
    float x, float y, float size, float alpha) {
    if (alpha >= 1.0f) {
        al_draw_scaled_bitmap(tiles, sx, sy, TILE_SRC, TILE_SRC,
            x, y, size, size, 0);
    } else {
        al_draw_tinted_scaled_bitmap(tiles,
            al_map_rgba_f(alpha, alpha, alpha, alpha),
            sx, sy, TILE_SRC, TILE_SRC, x, y, size, size, 0);
    }
}

/**
 * @brief Draws the block tile of a piece type.
 *
 * @param tiles Tiles sheet, or NULL for the colour fallback.
 * @param type Piece type (0-6).
 * @param x Destination x.
 * @param y Destination y.
 * @param size Destination side.
 * @param alpha Opacity (1 = opaque).
 */
void piece_draw_tile(ALLEGRO_BITMAP *tiles, int type, float x, float y,
    float size, float alpha) {
    static const int tile_x[PIECE_TYPES] = { 26, 42, 18, 34, 26, 2, 10 };
    static const int tile_y[PIECE_TYPES] = { 108, 100, 100, 100, 100, 100, 100 };

    if (type < 0 || type >= PIECE_TYPES) {
        return;
    }

    if (!tiles) {
        ALLEGRO_COLOR color = piece_get_color(type);
        al_draw_filled_rectangle(x, y, x + size, y + size, color);
        al_draw_filled_rectangle(x + 1.0f, y + 1.0f,
            x + size - 2.0f, y + size - 2.0f, color);
        return;
    }

    piece_blit(tiles, tile_x[type], tile_y[type], x, y, size, alpha);
}

bool piece_blocks_has(int blocks[4][2], int x, int y) {
    int i = 0;

    for (i = 0; i < 4; i++) {
        if (blocks[i][0] == x && blocks[i][1] == y) {
            return true;
        }
    }

    return false;
}

void piece_draw_ibeam(ALLEGRO_BITMAP *tiles, bool left, bool right,
    bool up, bool down, float x, float y, float size, float alpha) {
    static const int beam_hx[3] = { 308, 316, 332 }; // left, middle, right
    static const int beam_vy[3] = { 50, 58, 74 };    // top, middle, bottom
    int beam_hy = 66;
    int beam_vx = 351;
    bool horizontal = true;
    int seg = 1;

    if (left || right) {
        seg = !left ? 0 : (!right ? 2 : 1);
    } else if (up || down) {
        horizontal = false;
        seg = !up ? 0 : (!down ? 2 : 1);
    }

    if (!tiles) {
        piece_draw_tile(NULL, PIECE_I, x, y, size, alpha);
        return;
    }

    if (horizontal) {
        piece_blit(tiles, beam_hx[seg], beam_hy, x, y, size, alpha);
    } else {
        piece_blit(tiles, beam_vx, beam_vy[seg], x, y, size, alpha);
    }
}

/**
 * @brief Renders the piece on the board.
 *
 * @param piece Pointer to the piece.
 * @param tiles Tiles sheet, or NULL for the colour fallback.
 * @param ghost If true renders translucent.
 * @return OK on success, ERROR if piece is NULL.
 */
STATUS piece_print(PIECE *piece, ALLEGRO_BITMAP *tiles, bool ghost) {
    int blocks[4][2];
    int i = 0;
    float bx = 0.0f;
    float by = 0.0f;
    float cs = (float) CELL_SIZE;
    float x0 = (float) BOARD_X0;
    float y0 = (float) BOARD_Y0;
    float alpha = ghost ? 0.35f : 1.0f;

    if (!piece) {
        return ERROR;
    }

    piece_get_blocks(piece, blocks);

    for (i = 0; i < 4; i++) {
        int x = blocks[i][0];
        int y = blocks[i][1];

        if (y < BOARD_HIDDEN) {
            continue; // hidden rows, not rendered
        }

        bx = x0 + x * cs;
        by = y0 + (y - BOARD_HIDDEN) * cs;

        if (piece->type == PIECE_I) {
            piece_draw_ibeam(tiles,
                piece_blocks_has(blocks, x - 1, y),
                piece_blocks_has(blocks, x + 1, y),
                piece_blocks_has(blocks, x, y - 1),
                piece_blocks_has(blocks, x, y + 1),
                bx, by, cs, alpha);
        } else {
            piece_draw_tile(tiles, piece->type, bx, by, cs, alpha);
        }
    }

    return OK;
}
