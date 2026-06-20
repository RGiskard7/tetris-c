/**
 * @file game.c
 * @brief Core game logic: state machine, input, gravity, scoring and rendering.
 *
 * Implements classic Tetris: seven tetrominoes, level-based gravity,
 * delayed auto-shift (DAS), lock delay, ghost piece, line clearing
 * with NES-style scoring, next-piece preview and a four-state
 * state machine (title, play, pause, game over).
 *
 * Author: RGiskard7
 * Date: 20/06/2026
 */

#include "game.h"
#include "config.h"
#include "board.h"
#include "piece.h"

#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>
#include <allegro5/allegro_audio.h>
#include <allegro5/allegro_acodec.h>
#include <stdio.h>
#include <stdlib.h>

struct _game {
    ALLEGRO_DISPLAY *display;           ///< Game window
    ALLEGRO_TIMER *timer;               ///< Frame timer (FPS)
    ALLEGRO_EVENT_QUEUE *event_queue;   ///< Event queue
    ALLEGRO_EVENT events;               ///< Last event polled
    ALLEGRO_FONT *font;                 ///< TTF font for HUD and overlays
    ALLEGRO_SAMPLE *music;              ///< Background music sample
    ALLEGRO_SAMPLE_ID music_id;         ///< ID of the playing music instance
    bool music_playing;                 ///< True while music is active

    BOARD *board;                       ///< Locked-cell grid
    PIECE *piece;                       ///< Active falling tetromino
    int next_type;                      ///< Type of the upcoming piece

    GAME_STATE state;                   ///< Current state machine state
    bool done;                          ///< True when the main loop must exit
    bool draw;                          ///< True when a redraw is pending

    int score;                          ///< Player score
    int level;                          ///< Current level (0-based)
    int lines;                          ///< Total lines cleared

    int gravity_timer;                  ///< Frames since last gravity tick
    int lock_timer;                     ///< Frames the piece has been on the ground
    bool piece_on_ground;               ///< True when piece can't move down

    int das_timer;                      ///< Frames remaining until DAS repeat
    int das_dir;                        ///< DIR_LEFT or DIR_RIGHT when DAS is active

    int title_timer;                    ///< Counter for blinking title text
    bool enter_was_down;                ///< Edge detection for ENTER
    bool pause_was_down;                ///< Edge detection for P
    bool up_was_down;                   ///< Edge detection for rotate
    bool space_was_down;                ///< Edge detection for hard drop
};

/**
 * @brief Level speed table (frames per gravity tick @ 60 FPS).
 */
static const int _level_speeds[MAX_LEVEL + 1] = { SPEED_TABLE };

/**
 * @brief Point values for 1, 2, 3 and 4 lines cleared.
 */
static const int _line_scores[4] = {
    PTS_SINGLE, PTS_DOUBLE, PTS_TRIPLE, PTS_TETRIS
};

// Private function declarations

static STATUS game_new_game(GAME *game);
static STATUS game_spawn_piece(GAME *game);
static int    game_random_piece(GAME *game, int last_type);
static int    game_get_gravity(GAME *game);
static void   game_add_score(GAME *game, int lines_cleared);
static void   game_move_ghost(GAME *game, PIECE *ghost);
static STATUS game_update_play(GAME *game, ALLEGRO_KEYBOARD_STATE *key);

static STATUS game_print_hud(GAME *game);
static STATUS game_print_preview(GAME *game);
static STATUS game_print_overlay(GAME *game);

// =========================================================================
// Functions: Game Creation and Destruction
// =========================================================================

/**
 * @brief Allocates a game and sets every field to a safe default.
 *
 * @return Pointer to the created game or NULL if allocation fails.
 */
GAME *game_create(void) {
    GAME *new_game = NULL;

    new_game = (GAME *) malloc(sizeof (GAME));
    if (!new_game) {
        return NULL;
    }

    new_game->display = NULL;
    new_game->timer = NULL;
    new_game->event_queue = NULL;
    new_game->font = NULL;
    new_game->music = NULL;
    new_game->music_playing = false;
    new_game->board = NULL;
    new_game->piece = NULL;

    new_game->next_type = 0;

    new_game->state = STATE_TITLE;
    new_game->done = false;
    new_game->draw = false;

    new_game->score = 0;
    new_game->level = 0;
    new_game->lines = 0;

    new_game->gravity_timer = 0;
    new_game->lock_timer = 0;
    new_game->piece_on_ground = false;

    new_game->das_timer = 0;
    new_game->das_dir = DIR_NONE;

    new_game->title_timer = 0;
    new_game->enter_was_down = true;
    new_game->pause_was_down = true;
    new_game->up_was_down = true;
    new_game->space_was_down = true;

    return new_game;
}

/**
 * @brief Destroys a game and every resource it owns.
 *
 * @param game Pointer to the game to destroy.
 */
void game_destroy(GAME *game) {
    if (!game) {
        return;
    }

    if (game->piece) {
        piece_destroy(game->piece);
        game->piece = NULL;
    }
    if (game->board) {
        board_destroy(game->board);
        game->board = NULL;
    }
    if (game->music) {
        al_stop_sample(&game->music_id);
        al_destroy_sample(game->music);
        game->music = NULL;
    }
    if (game->font) {
        al_destroy_font(game->font);
        game->font = NULL;
    }
    if (game->event_queue) {
        al_destroy_event_queue(game->event_queue);
        game->event_queue = NULL;
    }
    if (game->timer) {
        al_destroy_timer(game->timer);
        game->timer = NULL;
    }
    if (game->display) {
        al_destroy_display(game->display);
        game->display = NULL;
    }

    free(game);
}

// =========================================================================
// Functions: Game Initialization
// =========================================================================

/**
 * @brief Initializes the window, timer, queue, font, board and piece.
 *
 * @param game Pointer to the game.
 * @return STATUS code (OK on success, ERROR on any failure).
 */
STATUS game_init(GAME *game) {
    if (!game) {
        return ERROR;
    }

    game->timer = al_create_timer(1.0 / FPS);
    if (!game->timer) {
        return ERROR;
    }

    game->display = al_create_display(DISPLAY_WIDTH, DISPLAY_HEIGHT);
    if (!game->display) {
        return ERROR;
    }
    al_set_window_title(game->display, "Tetris");

    game->event_queue = al_create_event_queue();
    if (!game->event_queue) {
        return ERROR;
    }

    al_register_event_source(game->event_queue,
        al_get_timer_event_source(game->timer));
    al_register_event_source(game->event_queue,
        al_get_display_event_source(game->display));
    al_register_event_source(game->event_queue,
        al_get_keyboard_event_source());

    // la fuente y la musica son opcionales: si faltan, el juego sigue
    game->font = al_load_ttf_font(FONT_RSC, 18, 0);
    game->music = al_load_sample(SND_MUSIC);

    game->board = board_create();
    if (!game->board) {
        return ERROR;
    }

    game->piece = piece_create();
    if (!game->piece) {
        return ERROR;
    }

    return OK;
}

// =========================================================================
// Functions: Accessors
// =========================================================================

/**
 * @brief Retrieves the game display.
 *
 * @param game Pointer to the game.
 * @return Display pointer, or NULL if game is NULL.
 */
ALLEGRO_DISPLAY *game_get_display(GAME *game) {
    if (!game) {
        return NULL;
    }

    return game->display;
}

/**
 * @brief Retrieves the game timer.
 *
 * @param game Pointer to the game.
 * @return Timer pointer, or NULL if game is NULL.
 */
ALLEGRO_TIMER *game_get_timer(GAME *game) {
    if (!game) {
        return NULL;
    }

    return game->timer;
}

/**
 * @brief Retrieves the game event queue.
 *
 * @param game Pointer to the game.
 * @return Event queue pointer, or NULL if game is NULL.
 */
ALLEGRO_EVENT_QUEUE *game_get_queue(GAME *game) {
    if (!game) {
        return NULL;
    }

    return game->event_queue;
}

/**
 * @brief Retrieves the address of the game's event slot.
 *
 * @param game Pointer to the game.
 * @return Event pointer, or NULL if game is NULL.
 */
ALLEGRO_EVENT *game_get_event(GAME *game) {
    if (!game) {
        return NULL;
    }

    return &game->events;
}

/**
 * @brief Tells whether the main loop should stop.
 *
 * @param game Pointer to the game.
 * @return True if done, true also if game is NULL.
 */
bool game_is_done(GAME *game) {
    if (!game) {
        return true;
    }

    return game->done;
}

// =========================================================================
// Functions: Helpers
// =========================================================================

/**
 * @brief Starts a brand new game: resets score, level, lines, board and
 *        spawns the first piece.
 *
 * @param game Pointer to the game.
 * @return STATUS code (OK on success, ERROR on failure).
 */
static STATUS game_new_game(GAME *game) {
    if (!game) {
        return ERROR;
    }

    game->score = 0;
    game->level = 0;
    game->lines = 0;
    game->gravity_timer = 0;
    game->lock_timer = 0;
    game->piece_on_ground = false;
    game->up_was_down = true;
    game->space_was_down = true;

    board_clear(game->board);

    piece_reset(game->piece);
    game->next_type = game_random_piece(game, piece_get_type(game->piece));

    return game_spawn_piece(game);
}

/**
 * @brief NES-style randomiser: rolls a piece. If it matches the last one,
 *        rolls again once. Reduces long streaks of the same piece.
 *
 * @param game Pointer to the game (unused, kept for API consistency).
 * @param last_type Previous piece type to avoid repeating.
 * @return A piece type (0-6).
 */
static int game_random_piece(GAME *game, int last_type) {
    int type = 0;

    (void) game;

    type = rand() % PIECE_TYPES;
    if (type == last_type) {
        type = rand() % PIECE_TYPES; // reroll once
    }

    return type;
}

/**
 * @brief Spawns the next piece. If it collides immediately, the game is over.
 *
 * @param game Pointer to the game.
 * @return STATUS code (OK on success, ERROR if game-over).
 */
static STATUS game_spawn_piece(GAME *game) {
    int blocks[4][2];

    if (!game) {
        return ERROR;
    }

    piece_set_type(game->piece, game->next_type);
    piece_set_rotation(game->piece, 0);
    piece_set_x(game->piece, SPAWN_X);
    piece_set_y(game->piece, SPAWN_Y);

    piece_get_blocks(game->piece, blocks);

    if (board_check_collision(game->board, blocks)) {
        game->state = STATE_OVER;
        if (game->music_playing) {
            al_stop_sample(&game->music_id);
            game->music_playing = false;
        }

        return ERROR;
    }

    game->next_type = game_random_piece(game, piece_get_type(game->piece));
    game->gravity_timer = 0;
    game->lock_timer = 0;
    game->piece_on_ground = false;

    return OK;
}

/**
 * @brief Returns the number of frames between gravity drops for the
 *        current level.
 *
 * @param game Pointer to the game.
 * @return Frames per gravity tick.
 */
static int game_get_gravity(GAME *game) {
    if (!game) {
        return 48;
    }

    if (game->level > MAX_LEVEL) {
        return _level_speeds[MAX_LEVEL];
    }

    return _level_speeds[game->level];
}

/**
 * @brief Adds score based on lines cleared and the current level.
 *        Also updates level and lines count.
 *
 * @param game Pointer to the game.
 * @param lines_cleared Number of lines cleared (1-4).
 */
static void game_add_score(GAME *game, int lines_cleared) {
    if (!game || lines_cleared < 1 || lines_cleared > 4) {
        return;
    }

    game->score += _line_scores[lines_cleared - 1] * (game->level + 1);
    game->lines += lines_cleared;
    game->level = game->lines / LINES_PER_LEVEL;
    if (game->level > MAX_LEVEL) {
        game->level = MAX_LEVEL;
    }
}

/**
 * @brief Moves a ghost piece down until it collides with the board.
 *
 * @param game Pointer to the game.
 * @param ghost Piece to modify (starts from the active piece state).
 */
static void game_move_ghost(GAME *game, PIECE *ghost) {
    int blocks[4][2];

    if (!game || !ghost) {
        return;
    }

    while (true) {
        piece_move(ghost, 0, 1);
        piece_get_blocks(ghost, blocks);
        if (board_check_collision(game->board, blocks)) {
            piece_move(ghost, 0, -1);
            break;
        }
    }
}

// =========================================================================
// Functions: Game Update
// =========================================================================

/**
 * @brief Advances the game one frame according to the current state.
 *
 * @param game Pointer to the game.
 * @param key Current keyboard state.
 * @return STATUS code (OK on success, ERROR on bad arguments).
 */
STATUS game_update(GAME *game, ALLEGRO_KEYBOARD_STATE *key) {
    bool enter_now = false;

    if (!game || !key) {
        return ERROR;
    }

    // ESC sale del juego salvo durante la partida (donde pausa)
    if (game->state != STATE_PLAY && al_key_down(key, ALLEGRO_KEY_ESCAPE)) {
        game->done = true;
        return OK;
    }

    switch (game->state) {
        case STATE_TITLE:
            game->title_timer++;
            enter_now = al_key_down(key, ALLEGRO_KEY_ENTER);
            if (enter_now && !game->enter_was_down) {
                if (game_new_game(game) == OK) {
                    game->state = STATE_PLAY;
                    if (game->music && !game->music_playing) {
                        al_play_sample(game->music, 0.4f, 0, 1.0f,
                            ALLEGRO_PLAYMODE_LOOP, &game->music_id);
                        game->music_playing = true;
                    }
                }
            }
            game->enter_was_down = enter_now;
            break;

        case STATE_PLAY:
            game_update_play(game, key);
            break;

        case STATE_PAUSE: {
            bool p_now = al_key_down(key, ALLEGRO_KEY_P);
            if (p_now && !game->pause_was_down) {
                game->state = STATE_PLAY;
                if (game->music && !game->music_playing) {
                    al_play_sample(game->music, 0.4f, 0, 1.0f,
                        ALLEGRO_PLAYMODE_LOOP, &game->music_id);
                    game->music_playing = true;
                }
            }
            game->pause_was_down = p_now;
            break;
        }

        case STATE_OVER:
            enter_now = al_key_down(key, ALLEGRO_KEY_ENTER);
            if (enter_now && !game->enter_was_down) {
                game->state = STATE_TITLE;
            }
            game->enter_was_down = enter_now;
            break;

        default:
            break;
    }

    game->draw = true;

    return OK;
}

/**
 * @brief Handles input, gravity and locking during active play.
 *
 * @param game Pointer to the game.
 * @param key Current keyboard state.
 * @return STATUS code (OK on success, ERROR on bad arguments).
 */
static STATUS game_update_play(GAME *game, ALLEGRO_KEYBOARD_STATE *key) {
    int blocks[4][2];
    bool left = false;
    bool right = false;
    bool down = false;
    bool space = false;
    bool up = false;
    bool p_now = false;
    int cleared = 0;

    if (!game || !key) {
        return ERROR;
    }

    // pausa con deteccion de flanco
    p_now = al_key_down(key, ALLEGRO_KEY_P);
    if (p_now && !game->pause_was_down) {
        game->state = STATE_PAUSE;
        game->pause_was_down = p_now;
        if (game->music_playing) {
            al_stop_sample(&game->music_id);
            game->music_playing = false;
        }
        return OK;
    }
    game->pause_was_down = p_now;

    // ESC tambien pausa durante la partida
    if (al_key_down(key, ALLEGRO_KEY_ESCAPE)) {
        game->state = STATE_PAUSE;
        if (game->music_playing) {
            al_stop_sample(&game->music_id);
            game->music_playing = false;
        }
        return OK;
    }

    left = al_key_down(key, ALLEGRO_KEY_LEFT);
    right = al_key_down(key, ALLEGRO_KEY_RIGHT);
    down = al_key_down(key, ALLEGRO_KEY_DOWN);
    space = al_key_down(key, ALLEGRO_KEY_SPACE);
    up = al_key_down(key, ALLEGRO_KEY_UP);

    // DAS (Delayed Auto Shift) para izquierda y derecha
    if (left && !right) {
        if (game->das_dir != DIR_LEFT || game->das_timer <= 0) {
            piece_move(game->piece, -1, 0);
            piece_get_blocks(game->piece, blocks);
            if (board_check_collision(game->board, blocks)) {
                piece_move(game->piece, 1, 0); // deshacer
            } else {
                game->lock_timer = 0;
            }
            if (game->das_dir == DIR_LEFT) {
                game->das_timer = DAS_REPEAT;
            } else {
                game->das_timer = DAS_DELAY;
            }
        }
        game->das_dir = DIR_LEFT;
        game->das_timer--;
    } else if (right && !left) {
        if (game->das_dir != DIR_RIGHT || game->das_timer <= 0) {
            piece_move(game->piece, 1, 0);
            piece_get_blocks(game->piece, blocks);
            if (board_check_collision(game->board, blocks)) {
                piece_move(game->piece, -1, 0); // deshacer
            } else {
                game->lock_timer = 0;
            }
            if (game->das_dir == DIR_RIGHT) {
                game->das_timer = DAS_REPEAT;
            } else {
                game->das_timer = DAS_DELAY;
            }
        }
        game->das_dir = DIR_RIGHT;
        game->das_timer--;
    } else {
        game->das_dir = DIR_NONE;
        game->das_timer = 0;
    }

    // rotacion con deteccion de flanco (una pulsacion = un giro)
    // NES original: sin wall kicks, la rotacion falla si hay colision
    if (up && !game->up_was_down) {
        int orig_rot = piece_get_rotation(game->piece);

        piece_rotate_cw(game->piece);
        piece_get_blocks(game->piece, blocks);

        if (board_check_collision(game->board, blocks)) {
            // rotacion falla, restaurar
            piece_set_rotation(game->piece, orig_rot);
        } else {
            // rotacion exitosa: reiniciar lock delay (NES behaviour)
            game->lock_timer = 0;
        }
    }
    game->up_was_down = up;

    // hard drop (SPACE): bajar instantaneamente y bloquear (flanco)
    if (space && !game->space_was_down) {
        while (true) {
            piece_move(game->piece, 0, 1);
            piece_get_blocks(game->piece, blocks);
            if (board_check_collision(game->board, blocks)) {
                piece_move(game->piece, 0, -1);
                break;
            }
            game->score += PTS_HARD_DROP;
        }

        // bloquear inmediatamente
        piece_get_blocks(game->piece, blocks);
        board_lock_piece(game->board, blocks,
            piece_get_color(piece_get_type(game->piece)));
        cleared = board_clear_lines(game->board);
        if (cleared > 0) {
            game_add_score(game, cleared);
        }
        if (board_is_game_over(game->board)) {
            game->state = STATE_OVER;
            if (game->music_playing) {
                al_stop_sample(&game->music_id);
                game->music_playing = false;
            }
        } else {
            game_spawn_piece(game);
        }
        game->space_was_down = space;
        return OK;
    }

    // soft drop: bajar una fila por frame cuando se mantiene pulsado
    if (down) {
        piece_move(game->piece, 0, 1);
        piece_get_blocks(game->piece, blocks);
        if (board_check_collision(game->board, blocks)) {
            piece_move(game->piece, 0, -1); // no se puede, deshacer
        } else {
            game->score += PTS_SOFT_DROP;
        }
    }

    // gravedad
    game->gravity_timer++;
    if (game->gravity_timer >= game_get_gravity(game)) {
        game->gravity_timer = 0;

        piece_move(game->piece, 0, 1);
        piece_get_blocks(game->piece, blocks);

        if (board_check_collision(game->board, blocks)) {
            // la pieza toca suelo, deshacer el movimiento
            piece_move(game->piece, 0, -1);
            game->piece_on_ground = true;
        } else {
            game->piece_on_ground = false;
            game->lock_timer = 0;
        }
    }

    // lock delay: si la pieza esta en el suelo, esperar LOCK_DELAY frames
    if (game->piece_on_ground) {
        game->lock_timer++;
        if (game->lock_timer >= LOCK_DELAY) {
            piece_get_blocks(game->piece, blocks);
            board_lock_piece(game->board, blocks,
                piece_get_color(piece_get_type(game->piece)));
            cleared = board_clear_lines(game->board);
            if (cleared > 0) {
                game_add_score(game, cleared);
            }
            if (board_is_game_over(game->board)) {
                game->state = STATE_OVER;
                if (game->music_playing) {
                    al_stop_sample(&game->music_id);
                    game->music_playing = false;
                }
            } else {
                game_spawn_piece(game);
            }
        }
    }

    game->space_was_down = space;

    return OK;
}

// =========================================================================
// Functions: Rendering
// =========================================================================

/**
 * @brief Renders the current frame and flips the display.
 *
 * @param game Pointer to the game.
 * @return STATUS code (OK on success, ERROR if nothing to draw).
 */
STATUS game_render(GAME *game) {
    PIECE *ghost = NULL;

    if (!game || !game->draw) {
        return ERROR;
    }

    al_clear_to_color(COLOR_BG);

    board_print(game->board);

    if (game->state == STATE_PLAY || game->state == STATE_PAUSE) {
        // ghost piece
        ghost = piece_create();
        if (ghost) {
            piece_set_type(ghost, piece_get_type(game->piece));
            piece_set_rotation(ghost, piece_get_rotation(game->piece));
            piece_set_x(ghost, piece_get_x(game->piece));
            piece_set_y(ghost, piece_get_y(game->piece));
            game_move_ghost(game, ghost);
            piece_print(ghost, true);
            piece_destroy(ghost);
        }

        // active piece
        piece_print(game->piece, false);
    }

    game_print_preview(game);
    game_print_hud(game);
    game_print_overlay(game);

    al_flip_display();
    game->draw = false;

    return OK;
}

/**
 * @brief Draws the next-piece preview box.
 *
 * @param game Pointer to the game.
 * @return STATUS code (OK on success, ERROR if game is NULL).
 */
static STATUS game_print_preview(GAME *game) {
    int blocks[4][2];
    int i = 0;
    float bx = 0.0f;
    float by = 0.0f;
    float cs = (float) PREVIEW_CELL;
    float px = (float) PREVIEW_X;
    float py = (float) PREVIEW_Y;
    ALLEGRO_COLOR clr;

    if (!game) {
        return ERROR;
    }

    // preview background
    al_draw_filled_rectangle(px - BORDER_W, py - BORDER_W,
        px + 4.0f * cs + BORDER_W, py + 4.0f * cs + BORDER_W, COLOR_BORDER);
    al_draw_filled_rectangle(px, py,
        px + 4.0f * cs, py + 4.0f * cs, COLOR_BOARD_BG);

    // draw "NEXT" label
    if (game->font) {
        al_draw_text(game->font, al_map_rgb(200, 200, 200),
            px + 2.0f * cs, py - 22.0f, ALLEGRO_ALIGN_CENTER, "NEXT");
    }

    // draw the next piece centred in the preview box
    clr = piece_get_color(game->next_type);
    piece_get_blocks_at(game->next_type, 0, 0, 0, blocks);

    for (i = 0; i < 4; i++) {
        bx = px + (float) blocks[i][0] * cs;
        by = py + (float) blocks[i][1] * cs;
        al_draw_filled_rectangle(bx, by, bx + cs, by + cs, clr);
        al_draw_filled_rectangle(bx + 1.0f, by + 1.0f,
            bx + cs - 2.0f, by + cs - 2.0f, clr);
    }

    return OK;
}

/**
 * @brief Draws the HUD: score, level and lines.
 *
 * @param game Pointer to the game.
 * @return STATUS code (OK on success, ERROR if game or font is NULL).
 */
static STATUS game_print_hud(GAME *game) {
    char buf[16];
    ALLEGRO_COLOR c_white = al_map_rgb(255, 255, 255);

    if (!game) {
        return ERROR;
    }

    if (!game->font) {
        return OK;
    }

    // score
    al_draw_text(game->font, al_map_rgb(180, 180, 180),
        HUD_X, (float) SCORE_LABEL_Y, ALLEGRO_ALIGN_LEFT, "SCORE");
    sprintf(buf, "%d", game->score);
    al_draw_text(game->font, c_white,
        HUD_X, (float) SCORE_VALUE_Y, ALLEGRO_ALIGN_LEFT, buf);

    // level
    al_draw_text(game->font, al_map_rgb(180, 180, 180),
        HUD_X, (float) LEVEL_LABEL_Y, ALLEGRO_ALIGN_LEFT, "LEVEL");
    sprintf(buf, "%d", game->level);
    al_draw_text(game->font, c_white,
        HUD_X, (float) LEVEL_VALUE_Y, ALLEGRO_ALIGN_LEFT, buf);

    // lines
    al_draw_text(game->font, al_map_rgb(180, 180, 180),
        HUD_X, (float) LINES_LABEL_Y, ALLEGRO_ALIGN_LEFT, "LINES");
    sprintf(buf, "%d", game->lines);
    al_draw_text(game->font, c_white,
        HUD_X, (float) LINES_VALUE_Y, ALLEGRO_ALIGN_LEFT, buf);

    return OK;
}

/**
 * @brief Draws the state-dependent overlay text (title, pause, game over).
 *
 * @param game Pointer to the game.
 * @return STATUS code (OK on success, ERROR if game or font is NULL).
 */
static STATUS game_print_overlay(GAME *game) {
    char buf[32];

    if (!game) {
        return ERROR;
    }

    if (!game->font) {
        return OK;
    }

    switch (game->state) {
        case STATE_TITLE:
            al_draw_text(game->font, al_map_rgb(255, 255, 255),
                (float) DISPLAY_WIDTH / 2.0f, 300.0f,
                ALLEGRO_ALIGN_CENTER, "TETRIS");
            al_draw_text(game->font, al_map_rgb(200, 200, 200),
                (float) DISPLAY_WIDTH / 2.0f, 340.0f,
                ALLEGRO_ALIGN_CENTER,
                "LEFT / RIGHT: move   DOWN: soft drop");
            al_draw_text(game->font, al_map_rgb(200, 200, 200),
                (float) DISPLAY_WIDTH / 2.0f, 365.0f,
                ALLEGRO_ALIGN_CENTER,
                "UP: rotate   SPACE: hard drop   P: pause");
            if ((game->title_timer / 30) % 2) {
                al_draw_text(game->font, al_map_rgb(255, 255, 0),
                    (float) DISPLAY_WIDTH / 2.0f, 410.0f,
                    ALLEGRO_ALIGN_CENTER, "PRESS ENTER");
            }
            break;

        case STATE_PAUSE:
            al_draw_text(game->font, al_map_rgb(255, 255, 0),
                (float) DISPLAY_WIDTH / 2.0f, 360.0f,
                ALLEGRO_ALIGN_CENTER, "PAUSED");
            al_draw_text(game->font, al_map_rgb(200, 200, 200),
                (float) DISPLAY_WIDTH / 2.0f, 390.0f,
                ALLEGRO_ALIGN_CENTER, "P to resume");
            break;

        case STATE_OVER:
            al_draw_text(game->font, al_map_rgb(255, 0, 0),
                (float) DISPLAY_WIDTH / 2.0f, 300.0f,
                ALLEGRO_ALIGN_CENTER, "GAME OVER");
            sprintf(buf, "SCORE %d", game->score);
            al_draw_text(game->font, al_map_rgb(255, 255, 255),
                (float) DISPLAY_WIDTH / 2.0f, 335.0f,
                ALLEGRO_ALIGN_CENTER, buf);
            al_draw_text(game->font, al_map_rgb(200, 200, 200),
                (float) DISPLAY_WIDTH / 2.0f, 370.0f,
                ALLEGRO_ALIGN_CENTER, "ENTER to continue");
            break;

        default:
            break;
    }

    return OK;
}
