#ifndef CLIENT_INPUT_H
#define CLIENT_INPUT_H

#include "shared/board.h"
#include <stdbool.h>

/*
Universal interface for output
*/

typedef enum {
    INPUT_NONE,
    INPUT_QUIT,
    INPUT_FIRE,
    INPUT_PLACED_SHIPS
} InputType;

typedef struct {
    int x;
    int y;
} GridPos;

typedef struct {
    InputType type;
    // Optional data
    union {
        GridPos grid_pos;
        InitialShipState ships[NUM_SHIPS];
    };
} InputData;

// Receives input, must be non-blocking.
// If no input, return { INPUT_NONE, 0, 0 }
extern InputData get_user_input(void);

// Called if the submitted ship placement was invalid.
// Should reset and let the user try again
extern void reset_staged_ships(void);

#endif
