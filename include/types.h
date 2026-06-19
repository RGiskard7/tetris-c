/**
 * @file types.h
 * @brief Shared type definitions for the Tetris game.
 *
 * Defines direction enumeration and STATUS return values used across
 * the codebase.
 *
 * Author: RGiskard7
 * Date: 20/06/2026
 */

#ifndef TYPES_H
#define TYPES_H

#include <stdbool.h>

enum {
    DIR_NONE,
    DIR_LEFT,
    DIR_RIGHT,
    DIR_DOWN
};

typedef enum {
    OK,
    ERROR
} STATUS;

#endif /* TYPES_H */
