/**
 * @file config.h
 * @brief Configuration file for the Tetris game.
 *
 * All geometry, colours and rules for a classic Tetris implementation.
 * Board: 10 columns x 20 visible rows (+ 2 hidden rows at the top).
 * The HUD shows the next piece preview, score, level and line count.
 *
 * Author: RGiskard7
 * Date: 20/06/2026
 */

#ifndef CONFIG_H
#define CONFIG_H

// Display
#define DISPLAY_WIDTH  640
#define DISPLAY_HEIGHT 700
#define FPS             60

// Resources
#define FONT_RSC       "resources/fonts/space_invaders.ttf"
#define SND_MUSIC      "resources/sounds/sound.mp3"
#define SND_TITLE      "resources/sounds/title_screen.mp3"
#define SND_GAMEOVER   "resources/sounds/game_over.mp3"
#define SND_NAME       "resources/sounds/name_entry.mp3"

// Game Boy sprite sheets
#define IMG_PLAYFIELDS "resources/images/Game Boy _ GBC - Tetris - Miscellaneous - Playfields.png"
#define IMG_TILES      "resources/images/Game Boy _ GBC - Tetris - Miscellaneous - Tetriminos & Block Tiles.png"

// Purple sheet background, made transparent on load
#define MASK_R           72
#define MASK_G           40
#define MASK_B           96

// Source block tile size (8x8 px)
#define TILE_SRC          8

// Empty well region used as the board background
#define WELL_SRC_X       20
#define WELL_SRC_Y      320
#define WELL_SRC_W       70
#define WELL_SRC_H      134

// Brick wall column, drawn on the left and mirrored on the right
#define WALL_SRC_X      173
#define WALL_SRC_Y       10
#define WALL_SRC_W        8
#define WALL_SRC_H      140
#define WALL_DST_W       28

// Board geometry (10 cols x 22 rows: 20 visible + 2 hidden at the top)
#define BOARD_COLS      10
#define BOARD_ROWS      22
#define BOARD_VISIBLE   20
#define BOARD_HIDDEN     2
#define CELL_SIZE       28
#define BOARD_X0        ((DISPLAY_WIDTH - BOARD_COLS * CELL_SIZE) / 2 - 80)
#define BOARD_Y0         80

// Board border
#define BORDER_W         4

// Preview box
#define PREVIEW_X       430
#define PREVIEW_Y       120
#define PREVIEW_CELL     22

// HUD text positions
#define HUD_X           430
#define HUD_VALUE_X     440
#define SCORE_LABEL_Y   280
#define SCORE_VALUE_Y   310
#define LEVEL_LABEL_Y   370
#define LEVEL_VALUE_Y   400
#define LINES_LABEL_Y   460
#define LINES_VALUE_Y   490

// Pieces
#define PIECE_TYPES      7
#define PIECE_SIZE       4
#define PIECE_I          0   // I (HERO): drawn as a continuous beam, not tiles
#define SPAWN_X          (BOARD_COLS / 2 - 2)
#define SPAWN_Y          0

// Scoring (NES original, multiplied by current level + 1)
// Single = 40, Double = 100, Triple = 300, Tetris = 1200
#define PTS_SINGLE       40
#define PTS_DOUBLE      100
#define PTS_TRIPLE      300
#define PTS_TETRIS     1200
#define PTS_SOFT_DROP     1
#define PTS_HARD_DROP     2

// Level progression (10 lines per level, 30 levels 0-29)
#define LINES_PER_LEVEL  10
#define MAX_LEVEL        29

// DAS (Delayed Auto Shift) in frames
#define DAS_DELAY        16
#define DAS_REPEAT        6

// Lock delay in frames (brief, as in NES)
#define LOCK_DELAY       15

// Line clear flash: frames the completed rows blink before being removed
#define LINE_CLEAR_DELAY 18

// Colours
#define COLOR_BG        al_map_rgb(0, 0, 0)
#define COLOR_BOARD_BG  al_map_rgb(20, 20, 20)
#define COLOR_GRID      al_map_rgb(45, 45, 45)
#define COLOR_BORDER    al_map_rgb(100, 100, 100)
#define COLOR_GHOST     al_map_rgb(60, 60, 60)

// Tetromino colours (standard)
#define COLOR_I         al_map_rgb(0, 240, 240)
#define COLOR_O         al_map_rgb(240, 240, 0)
#define COLOR_T         al_map_rgb(160, 0, 240)
#define COLOR_S         al_map_rgb(0, 240, 0)
#define COLOR_Z         al_map_rgb(240, 0, 0)
#define COLOR_J         al_map_rgb(0, 0, 240)
#define COLOR_L         al_map_rgb(240, 160, 0)

// Level speed table (frames per grid-cell drop @ 60 FPS)
// NES original: 30 levels (0-29), level 29 is the "kill screen"
#define SPEED_TABLE                                   \
    48, 43, 38, 33, 28, 23, 18, 13, 8, 6,            \
     5,  5,  5,  4,  4,  4,  3,  3, 3, 2,            \
     2,  2,  2,  2,  2,  2,  2,  2, 2, 1

// High scores
#define TOP_SCORES_FILE "highscores.dat"
#define MAX_TOP_SCORES  5

typedef struct {
    char name[4];
    int score;
} TOP_ENTRY;

#endif /* CONFIG_H */
