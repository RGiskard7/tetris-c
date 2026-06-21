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
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_audio.h>
#include <allegro5/allegro_acodec.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct _game {
    ALLEGRO_DISPLAY *display;           ///< Game window
    ALLEGRO_TIMER *timer;               ///< Frame timer (FPS)
    ALLEGRO_EVENT_QUEUE *event_queue;   ///< Event queue
    ALLEGRO_EVENT events;               ///< Last event polled
    ALLEGRO_FONT *font;                 ///< TTF font for HUD and overlays
    ALLEGRO_BITMAP *playfield;          ///< GB playfields sheet (well bg)
    ALLEGRO_BITMAP *tiles;              ///< GB tetromino block tiles sheet
    ALLEGRO_SAMPLE *music;              ///< In-game background music
    ALLEGRO_SAMPLE *snd_title;          ///< Title screen music (loops)
    ALLEGRO_SAMPLE *snd_gameover;       ///< Game over jingle (plays once)
    ALLEGRO_SAMPLE *snd_name;           ///< Name entry music (loops)
    ALLEGRO_SAMPLE_ID music_id;         ///< ID of the current looping track
    bool music_playing;                 ///< True while a looping track is active
    int audio_state;                    ///< State whose audio is currently set

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

    TOP_ENTRY top_scores[MAX_TOP_SCORES + 1]; ///< Top 5 high scores (+1 for insert)
    bool hs_entry_active;               ///< True when entering initials
    int hs_entry_pos;                   ///< Current letter position (0-2)
    int hs_entry_cursor_timer;          ///< Timer for blinking cursor
    char hs_letters[3];                 ///< Letters being entered
    int hs_entry_delay;                 ///< Input delay frames
    bool hs_enter_needs_release;        ///< Require Enter release
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

static STATUS game_load_top_scores(GAME *game);
static STATUS game_save_top_scores(GAME *game);
static void   game_insert_top_score(GAME *game, int score);
static int    game_highscore_verify(GAME *game, int score);
static STATUS game_new_game(GAME *game);
static STATUS game_spawn_piece(GAME *game);
static int    game_random_piece(GAME *game, int last_type);
static int    game_get_gravity(GAME *game);
static void   game_add_score(GAME *game, int lines_cleared);
static void   game_move_ghost(GAME *game, PIECE *ghost);
static void   game_music_stop(GAME *game);
static void   game_music_loop(GAME *game, ALLEGRO_SAMPLE *sample);
static void   game_update_audio(GAME *game);
static STATUS game_update_play(GAME *game, ALLEGRO_KEYBOARD_STATE *key);

static STATUS game_update_highscore(GAME *game, ALLEGRO_KEYBOARD_STATE *key);

static STATUS game_print_hud(GAME *game);
static STATUS game_print_preview(GAME *game);
static STATUS game_print_overlay(GAME *game);

/**
 * @brief Loads top scores from the persistence file.
 *
 * @param game Pointer to the game.
 * @return OK on success, ERROR if game is NULL.
 */
static STATUS game_load_top_scores(GAME *game) {
    int i = 0;

    if (!game) {
        return ERROR;
    }

    FILE *f = fopen(TOP_SCORES_FILE, "r");
    if (!f) {
        return OK;
    }

    for (i = 0; i <= MAX_TOP_SCORES; i++) {
        if (fscanf(f, "%3s %d", game->top_scores[i].name,
            &game->top_scores[i].score) != 2) {
            break;
        }
    }

    fclose(f);

    return OK;
}

/**
 * @brief Saves top scores to the persistence file.
 *
 * @param game Pointer to the game.
 * @return OK on success, ERROR if game is NULL.
 */
static STATUS game_save_top_scores(GAME *game) {
    int i = 0;

    if (!game) {
        return ERROR;
    }

    FILE *f = fopen(TOP_SCORES_FILE, "w");
    if (!f) {
        return ERROR;
    }

    for (i = 0; i <= MAX_TOP_SCORES; i++) {
        fprintf(f, "%s %d\n", game->top_scores[i].name,
            game->top_scores[i].score);
    }

    fclose(f);

    return OK;
}

/**
 * @brief Inserts a score into the sorted top scores list.
 *
 * @param game Pointer to the game.
 * @param score Score to insert.
 */
static void game_insert_top_score(GAME *game, int score) {
    int i = 0;
    int j = 0;

    if (!game || score <= 0) {
        return;
    }

    for (i = 0; i <= MAX_TOP_SCORES; i++) {
        if (score > game->top_scores[i].score) {
            for (j = MAX_TOP_SCORES; j > i; j--) {
                game->top_scores[j] = game->top_scores[j - 1];
            }
            strcpy(game->top_scores[i].name, "---");
            game->top_scores[i].score = score;
            break;
        }
    }
}

/**
 * @brief Checks if the given score qualifies for the top scores list.
 *
 * @param game Pointer to the game.
 * @param score Score to check.
 * @return 1 if it qualifies, 0 otherwise.
 */
static int game_highscore_verify(GAME *game, int score) {
    int i = 0;

    if (!game || score <= 0) {
        return 0;
    }

    for (i = 0; i <= MAX_TOP_SCORES; i++) {
        if (score > game->top_scores[i].score) {
            return 1;
        }
    }

    return 0;
}

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
    new_game->playfield = NULL;
    new_game->tiles = NULL;
    new_game->music = NULL;
    new_game->snd_title = NULL;
    new_game->snd_gameover = NULL;
    new_game->snd_name = NULL;
    new_game->music_playing = false;
    new_game->audio_state = -1;
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

    for (int i = 0; i <= MAX_TOP_SCORES; i++) {
        strcpy(new_game->top_scores[i].name, "---");
        new_game->top_scores[i].score = 0;
    }
    new_game->hs_entry_active = false;

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
    if (game->music_playing) {
        al_stop_sample(&game->music_id);
        game->music_playing = false;
    }
    if (game->music) {
        al_destroy_sample(game->music);
        game->music = NULL;
    }
    if (game->snd_title) {
        al_destroy_sample(game->snd_title);
        game->snd_title = NULL;
    }
    if (game->snd_gameover) {
        al_destroy_sample(game->snd_gameover);
        game->snd_gameover = NULL;
    }
    if (game->snd_name) {
        al_destroy_sample(game->snd_name);
        game->snd_name = NULL;
    }
    if (game->font) {
        al_destroy_font(game->font);
        game->font = NULL;
    }
    if (game->tiles) {
        al_destroy_bitmap(game->tiles);
        game->tiles = NULL;
    }
    if (game->playfield) {
        al_destroy_bitmap(game->playfield);
        game->playfield = NULL;
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

    // la fuente, las imagenes y la musica son opcionales: si faltan,
    // el juego sigue (con un fallback de colores planos para los graficos)
    game->font = al_load_ttf_font(FONT_RSC, 18, 0);
    game->music = al_load_sample(SND_MUSIC);
    game->snd_title = al_load_sample(SND_TITLE);
    game->snd_gameover = al_load_sample(SND_GAMEOVER);
    game->snd_name = al_load_sample(SND_NAME);

    // Game Boy sprite sheets: load and make the purple background transparent
    game->playfield = al_load_bitmap(IMG_PLAYFIELDS);
    if (game->playfield) {
        al_convert_mask_to_alpha(game->playfield,
            al_map_rgb(MASK_R, MASK_G, MASK_B));
    }
    game->tiles = al_load_bitmap(IMG_TILES);
    if (game->tiles) {
        al_convert_mask_to_alpha(game->tiles,
            al_map_rgb(MASK_R, MASK_G, MASK_B));
    }

    game->board = board_create();
    if (!game->board) {
        return ERROR;
    }

    game->piece = piece_create();
    if (!game->piece) {
        return ERROR;
    }

    game_load_top_scores(game);

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
// Functions: Audio
// =========================================================================

/**
 * @brief Stops the looping background track if one is playing.
 *
 * @param game Pointer to the game.
 */
static void game_music_stop(GAME *game) {
    if (game && game->music_playing) {
        al_stop_sample(&game->music_id);
        game->music_playing = false;
    }
}

/**
 * @brief Starts a sample as the looping background track, replacing any
 *        track currently playing.
 *
 * @param game Pointer to the game.
 * @param sample Sample to loop, or NULL to leave silence.
 */
static void game_music_loop(GAME *game, ALLEGRO_SAMPLE *sample) {
    if (!game) {
        return;
    }

    game_music_stop(game);

    if (sample) {
        al_play_sample(sample, 0.4f, 0, 1.0f, ALLEGRO_PLAYMODE_LOOP,
            &game->music_id);
        game->music_playing = true;
    }
}

/**
 * @brief Keeps the soundtrack in sync with the current state.
 *
 * Each state owns its background sound: the title and name-entry screens
 * loop their music, play loops the in-game music, pause is silent, and
 * game over fires its jingle once. Driven by state changes so it only
 * acts on a transition.
 *
 * @param game Pointer to the game.
 */
static void game_update_audio(GAME *game) {
    if (!game || game->audio_state == (int) game->state) {
        return;
    }
    game->audio_state = (int) game->state;

    switch (game->state) {
        case STATE_TITLE:
            game_music_loop(game, game->snd_title);
            break;
        case STATE_PLAY:
            game_music_loop(game, game->music);
            break;
        case STATE_PAUSE:
            game_music_stop(game);
            break;
        case STATE_OVER:
            game_music_stop(game);
            if (game->snd_gameover) {
                al_play_sample(game->snd_gameover, 0.6f, 0, 1.0f,
                    ALLEGRO_PLAYMODE_ONCE, NULL);
            }
            break;
        case STATE_HIGHSCORE:
            game_music_loop(game, game->snd_name);
            break;
        default:
            break;
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
            }
            game->pause_was_down = p_now;
            break;
        }

        case STATE_OVER:
            enter_now = al_key_down(key, ALLEGRO_KEY_ENTER);
            if (enter_now && !game->enter_was_down) {
                if (game_highscore_verify(game, game->score)) {
                    game->hs_entry_active = true;
                    game->hs_entry_pos = 0;
                    memset(game->hs_letters, 'A', 3);
                    game->hs_entry_cursor_timer = 0;
                    game->hs_entry_delay = 45;
                    game->hs_enter_needs_release = true;
                    game_insert_top_score(game, game->score);
                    game->state = STATE_HIGHSCORE;
                } else {
                    game->state = STATE_TITLE;
                }
            }
            game->enter_was_down = enter_now;
            break;

        case STATE_HIGHSCORE:
            game_update_highscore(game, key);
            break;

        default:
            break;
    }

    game_update_audio(game);
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
        return OK;
    }
    game->pause_was_down = p_now;

    // ESC tambien pausa durante la partida
    if (al_key_down(key, ALLEGRO_KEY_ESCAPE)) {
        game->state = STATE_PAUSE;
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
            piece_get_type(game->piece));
        cleared = board_clear_lines(game->board);
        if (cleared > 0) {
            game_add_score(game, cleared);
        }
        if (board_is_game_over(game->board)) {
            game->state = STATE_OVER;
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
                piece_get_type(game->piece));
            cleared = board_clear_lines(game->board);
            if (cleared > 0) {
                game_add_score(game, cleared);
            }
            if (board_is_game_over(game->board)) {
                game->state = STATE_OVER;
            } else {
                game_spawn_piece(game);
            }
        }
    }

    game->space_was_down = space;

    return OK;
}

/**
 * @brief Handles input for the high score initials entry.
 *
 * UP/DOWN cycle letters, LEFT/RIGHT move cursor, ENTER saves, ESC skips.
 *
 * @param game Pointer to the game.
 * @param key Current keyboard state.
 * @return STATUS code (OK on success, ERROR on bad arguments).
 */
static STATUS game_update_highscore(GAME *game, ALLEGRO_KEYBOARD_STATE *key) {
    static bool up_held = false;
    static bool down_held = false;
    static bool right_held = false;
    static bool left_held = false;
    static bool enter_held = false;

    if (!game || !key) {
        return ERROR;
    }

    // input delay prevents Enter bleed from GAME_OVER screen
    if (game->hs_entry_delay > 0) {
        game->hs_entry_delay--;
        if (game->hs_entry_delay == 0) {
            up_held = down_held = right_held = left_held = true;
        }
        return OK;
    }

    game->hs_entry_cursor_timer++;
    if (game->hs_entry_cursor_timer >= 8) {
        game->hs_entry_cursor_timer = 0;
    }

    // UP: cycle letter forward
    if (al_key_down(key, ALLEGRO_KEY_UP)) {
        if (!up_held) {
            game->hs_letters[game->hs_entry_pos]++;
            if (game->hs_letters[game->hs_entry_pos] > 'Z') {
                game->hs_letters[game->hs_entry_pos] = '0';
            }
            up_held = true;
        }
    } else {
        up_held = false;
    }

    // DOWN: cycle letter backward
    if (al_key_down(key, ALLEGRO_KEY_DOWN)) {
        if (!down_held) {
            game->hs_letters[game->hs_entry_pos]--;
            if (game->hs_letters[game->hs_entry_pos] < '0') {
                game->hs_letters[game->hs_entry_pos] = 'Z';
            }
            down_held = true;
        }
    } else {
        down_held = false;
    }

    // RIGHT: next position
    if (al_key_down(key, ALLEGRO_KEY_RIGHT)) {
        if (!right_held) {
            game->hs_entry_pos++;
            if (game->hs_entry_pos > 2) {
                game->hs_entry_pos = 2;
            }
            right_held = true;
        }
    } else {
        right_held = false;
    }

    // LEFT: previous position
    if (al_key_down(key, ALLEGRO_KEY_LEFT)) {
        if (!left_held) {
            game->hs_entry_pos--;
            if (game->hs_entry_pos < 0) {
                game->hs_entry_pos = 0;
            }
            left_held = true;
        }
    } else {
        left_held = false;
    }

    // ENTER: save
    if (game->hs_enter_needs_release) {
        if (!al_key_down(key, ALLEGRO_KEY_ENTER)) {
            game->hs_enter_needs_release = false;
            enter_held = false;
        }
    } else if (al_key_down(key, ALLEGRO_KEY_ENTER)) {
        if (!enter_held) {
            game->top_scores[0].name[0] = game->hs_letters[0];
            game->top_scores[0].name[1] = game->hs_letters[1];
            game->top_scores[0].name[2] = game->hs_letters[2];
            game->top_scores[0].name[3] = '\0';
            game_save_top_scores(game);
            game->hs_entry_active = false;
            game->state = STATE_TITLE;
            game->enter_was_down = true;
        }
    } else {
        enter_held = false;
    }

    // ESC: skip
    if (al_key_down(key, ALLEGRO_KEY_ESCAPE)) {
        game->hs_entry_active = false;
        game->state = STATE_TITLE;
        game->enter_was_down = true;
    }

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

    board_print(game->board, game->playfield, game->tiles);

    if (game->state == STATE_PLAY || game->state == STATE_PAUSE) {
        // ghost piece
        ghost = piece_create();
        if (ghost) {
            piece_set_type(ghost, piece_get_type(game->piece));
            piece_set_rotation(ghost, piece_get_rotation(game->piece));
            piece_set_x(ghost, piece_get_x(game->piece));
            piece_set_y(ghost, piece_get_y(game->piece));
            game_move_ghost(game, ghost);
            piece_print(ghost, game->tiles, true);
            piece_destroy(ghost);
        }

        // active piece
        piece_print(game->piece, game->tiles, false);
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

    // draw the next piece centred in the preview box, with its GB tile
    piece_get_blocks_at(game->next_type, 0, 0, 0, blocks);

    for (i = 0; i < 4; i++) {
        bx = px + (float) blocks[i][0] * cs;
        by = py + (float) blocks[i][1] * cs;
        piece_draw_tile(game->tiles, game->next_type, bx, by, cs, 1.0f);
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
    int phase = 0;
    int i = 0;

    if (!game) {
        return ERROR;
    }

    if (!game->font) {
        return OK;
    }

    // dim the background so the overlay text stays readable over the light
    // Game Boy well (same approach as Space Invaders)
    if (game->state != STATE_PLAY) {
        al_draw_filled_rectangle(0, 0,
            (float) DISPLAY_WIDTH, (float) DISPLAY_HEIGHT,
            al_map_rgba(0, 0, 0, 200));
    }

    switch (game->state) {
        case STATE_TITLE:
            phase = (game->title_timer / 90) % 2;

            if (phase == 0) {
                al_draw_text(game->font, al_map_rgb(255, 255, 255),
                    (float) DISPLAY_WIDTH / 2.0f, 240.0f,
                    ALLEGRO_ALIGN_CENTER, "TETRIS");
                al_draw_text(game->font, al_map_rgb(200, 200, 200),
                    (float) DISPLAY_WIDTH / 2.0f, 290.0f,
                    ALLEGRO_ALIGN_CENTER,
                    "LEFT / RIGHT: move   DOWN: soft drop");
                al_draw_text(game->font, al_map_rgb(200, 200, 200),
                    (float) DISPLAY_WIDTH / 2.0f, 315.0f,
                    ALLEGRO_ALIGN_CENTER,
                    "UP: rotate   SPACE: hard drop   P: pause");
                if ((game->title_timer / 30) % 2) {
                    al_draw_text(game->font, al_map_rgb(255, 255, 0),
                        (float) DISPLAY_WIDTH / 2.0f, 370.0f,
                        ALLEGRO_ALIGN_CENTER, "PRESS ENTER");
                }
            } else {
                al_draw_text(game->font, al_map_rgb(255, 255, 0),
                    (float) DISPLAY_WIDTH / 2.0f, 140.0f,
                    ALLEGRO_ALIGN_CENTER, "TOP SCORES");
                for (i = 0; i < MAX_TOP_SCORES; i++) {
                    if (game->top_scores[i].score > 0) {
                        sprintf(buf, "%d.  %-3s  %d", i + 1,
                            game->top_scores[i].name,
                            game->top_scores[i].score);
                        al_draw_text(game->font, al_map_rgb(255, 255, 255),
                            (float) DISPLAY_WIDTH / 2.0f,
                            190.0f + i * 32.0f,
                            ALLEGRO_ALIGN_CENTER, buf);
                    }
                }
                if ((game->title_timer / 30) % 2) {
                    al_draw_text(game->font, al_map_rgb(255, 255, 0),
                        (float) DISPLAY_WIDTH / 2.0f, 380.0f,
                        ALLEGRO_ALIGN_CENTER, "PRESS ENTER");
                }
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

        case STATE_HIGHSCORE: {
            char letters[8];

            sprintf(buf, "SCORE: %d", game->score);
            sprintf(letters, "%c %c %c",
                game->hs_letters[0],
                game->hs_letters[1],
                game->hs_letters[2]);

            al_draw_text(game->font, al_map_rgb(0, 255, 0),
                (float) DISPLAY_WIDTH / 2.0f, 240.0f,
                ALLEGRO_ALIGN_CENTER, "NEW HIGH SCORE!");
            al_draw_text(game->font, al_map_rgb(255, 255, 255),
                (float) DISPLAY_WIDTH / 2.0f, 275.0f,
                ALLEGRO_ALIGN_CENTER, buf);
            al_draw_text(game->font, al_map_rgb(255, 255, 0),
                (float) DISPLAY_WIDTH / 2.0f, 310.0f,
                ALLEGRO_ALIGN_CENTER, "ENTER YOUR INITIALS:");
            al_draw_text(game->font, al_map_rgb(255, 255, 255),
                (float) DISPLAY_WIDTH / 2.0f, 345.0f,
                ALLEGRO_ALIGN_CENTER, letters);

            // blinking cursor
            if ((game->hs_entry_cursor_timer / 4) % 2 == 0) {
                int cx = DISPLAY_WIDTH / 2 - 20
                    + game->hs_entry_pos * 20;
                al_draw_text(game->font, al_map_rgb(255, 255, 255),
                    (float) cx, 362.0f,
                    ALLEGRO_ALIGN_CENTER, "_");
            }

            al_draw_text(game->font, al_map_rgb(180, 180, 180),
                (float) DISPLAY_WIDTH / 2.0f, 395.0f,
                ALLEGRO_ALIGN_CENTER,
                "UP / DOWN: change letter");
            al_draw_text(game->font, al_map_rgb(180, 180, 180),
                (float) DISPLAY_WIDTH / 2.0f, 420.0f,
                ALLEGRO_ALIGN_CENTER,
                "ENTER: save   ESC: skip");
            break;
        }

        default:
            break;
    }

    return OK;
}
