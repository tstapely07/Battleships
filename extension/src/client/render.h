#ifndef CLIENT_RENDER_H
#define CLIENT_RENDER_H

#include "state.h"
#include <stdbool.h>

/*
Universal interface for output
*/

/* Initialises window, loads textures/prepares terminal.
   Returns true on success, false on fatal error. */
extern bool init_graphics(void);

/* Checks if render backend is still active.
   Text: returns true until user force quits.
   Raylib: ... */
extern bool is_window_open(void);

/* Takes current state of the client and draws it.
   Text: Clears terminal and prints the boards
   Raylib: ... */
extern void render_frame(const ClientState *state);

/* Frees textures, closes windows, resets terminal. */
extern void cleanup_graphics(void);

#endif
