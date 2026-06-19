#ifndef GAME_H
#define GAME_H

#include <allegro5/allegro.h>
#include "types.h"

typedef struct _game GAME;

typedef enum {
    STATE_TITLE,
    STATE_PLAY,
    STATE_PAUSE,
    STATE_OVER
} GAME_STATE;

GAME *game_create(void);
STATUS game_init(GAME *g);
void   game_destroy(GAME *g);

ALLEGRO_DISPLAY     *game_get_display(GAME *g);
ALLEGRO_TIMER       *game_get_timer(GAME *g);
ALLEGRO_EVENT_QUEUE *game_get_queue(GAME *g);
ALLEGRO_EVENT       *game_get_event(GAME *g);

STATUS game_update(GAME *g, ALLEGRO_KEYBOARD_STATE *key);
STATUS game_render(GAME *g);
bool   game_is_done(GAME *g);

#endif
