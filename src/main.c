/**
 * @file main.c
 * @brief Main entry point for the Tetris game.
 *
 * Initializes Allegro and its addons, creates the game instance,
 * configures the window, and runs the main game loop.
 *
 * Author: RGiskard7
 * Date: 20/06/2026
 */

#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_audio.h>
#include <allegro5/allegro_acodec.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "game.h"
#include "config.h"

/**
 * @brief Structure to store initialization flags for Allegro components.
 *
 * Keeps track of which modules have been successfully initialized,
 * allowing proper cleanup on error.
 */
typedef struct {
  bool is_primitives_initialized;
  bool is_keyboard_initialized;
  bool is_font_initialized;
  bool is_ttf_initialized;
  bool is_image_initialized;
  bool is_audio_initialized;
} Flags;

void clean_up(Flags *flags, GAME *game);
bool init_allegro(Flags *flags);
void window_configuration(GAME *game);

/**
 * @brief Main entry point for the game.
 *
 * Initializes Allegro, creates the game instance, and starts
 * the main game loop.
 *
 * @return EXIT_SUCCESS on successful execution, EXIT_FAILURE otherwise.
 */
int main(void) {
  GAME *game = NULL;
  Flags flags = {false, false, false, false, false, false};
  ALLEGRO_KEYBOARD_STATE key;

  srand((unsigned)time(NULL));

  if (!init_allegro(&flags)) {
    clean_up(&flags, NULL);
    return EXIT_FAILURE;
  }

  game = game_create();
  if (!game) {
    clean_up(&flags, NULL);
    fprintf(stderr, "Error creating game instance.\n");
    return EXIT_FAILURE;
  }

  if (game_init(game) == ERROR) {
    clean_up(&flags, game);
    fprintf(stderr, "Error initializing game.\n");
    return EXIT_FAILURE;
  }

  window_configuration(game);

  al_start_timer(game_get_timer(game));

  while (game_is_done(game) != true) {
    al_wait_for_event(game_get_queue(game), game_get_event(game));
    al_get_keyboard_state(&key);

    if (game_get_event(game)->type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
      break;
    }

    if (game_get_event(game)->type == ALLEGRO_EVENT_TIMER) {
      if (game_update(game, &key) == ERROR) {
        clean_up(&flags, game);
        fprintf(stderr, "Error during update.\n");
        return EXIT_FAILURE;
      }

      if (game_render(game) == ERROR) {
        clean_up(&flags, game);
        fprintf(stderr, "Error rendering.\n");
        return EXIT_FAILURE;
      }
    }
  }

  clean_up(&flags, game);
  return EXIT_SUCCESS;
}

/**
 * @brief Initializes Allegro and its required addons.
 *
 * Initializes core Allegro and modules for primitives, keyboard,
 * fonts, TTF, images and audio.
 *
 * @param flags Pointer to Flags structure to mark successful init.
 * @return true if all modules initialized successfully, false otherwise.
 */
bool init_allegro(Flags *flags) {
  if (!al_init()) {
    fprintf(stderr, "Error initializing Allegro.\n");
    return false;
  }

  if (!al_init_primitives_addon()) {
    fprintf(stderr, "Error initializing Allegro Primitives Addon.\n");
    return false;
  }
  flags->is_primitives_initialized = true;

  if (!al_install_keyboard()) {
    fprintf(stderr, "Error installing keyboard.\n");
    return false;
  }
  flags->is_keyboard_initialized = true;

  if (!al_init_font_addon()) {
    fprintf(stderr, "Error initializing Allegro Font Addon.\n");
    return false;
  }
  flags->is_font_initialized = true;

  if (!al_init_ttf_addon()) {
    fprintf(stderr, "Error initializing Allegro TTF Addon.\n");
    return false;
  }
  flags->is_ttf_initialized = true;

  if (!al_init_image_addon()) {
    fprintf(stderr, "Error initializing Allegro Image Addon.\n");
    return false;
  }
  flags->is_image_initialized = true;

  if (!al_install_audio()) {
    fprintf(stderr, "Error installing Allegro Audio.\n");
    return false;
  }

  if (!al_init_acodec_addon()) {
    fprintf(stderr, "Error initializing Allegro Acodec Addon.\n");
    return false;
  }

  if (!al_reserve_samples(2)) {
    fprintf(stderr, "Error reserving samples.\n");
    return false;
  }
  flags->is_audio_initialized = true;

  return true;
}

/**
 * @brief Configures window positioning and title.
 *
 * Centers the game window on the primary monitor.
 *
 * @param game Pointer to the GAME instance.
 */
void window_configuration(GAME *game) {
  ALLEGRO_MONITOR_INFO monitor_info;
  int monitor_width, monitor_height;
  int window_pos_x, window_pos_y;

  al_get_monitor_info(0, &monitor_info);

  monitor_width = monitor_info.x2 - monitor_info.x1;
  monitor_height = monitor_info.y2 - monitor_info.y1;

  window_pos_x = (monitor_width - DISPLAY_WIDTH) / 2;
  window_pos_y = (monitor_height - DISPLAY_HEIGHT) / 2;

  al_set_window_position(game_get_display(game), window_pos_x, window_pos_y);
}

/**
 * @brief Cleans up allocated resources and shuts down Allegro.
 *
 * Destroys the game instance and shuts down Allegro modules
 * based on initialization flags.
 *
 * @param flags Pointer to Flags structure indicating initialized modules.
 * @param game Pointer to GAME instance to be destroyed, if not NULL.
 */
void clean_up(Flags *flags, GAME *game) {
  if (game) {
    game_destroy(game);
  }

  if (flags->is_audio_initialized) {
    al_uninstall_audio();
  }

  if (flags->is_image_initialized) {
    al_shutdown_image_addon();
  }

  if (flags->is_ttf_initialized) {
    al_shutdown_ttf_addon();
  }

  if (flags->is_font_initialized) {
    al_shutdown_font_addon();
  }

  if (flags->is_keyboard_initialized) {
    al_uninstall_keyboard();
  }

  if (flags->is_primitives_initialized) {
    al_shutdown_primitives_addon();
  }
}
