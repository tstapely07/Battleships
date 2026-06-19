#ifndef CLIENT_STATE_H
#define CLIENT_STATE_H

#include "shared/board.h"
#include <stdbool.h>

typedef enum {
    UI_STATE_CONNECTING,
    UI_STATE_PLACING_SHIPS,
    UI_STATE_WAITING_FOR_OPPONENT,
    UI_STATE_MY_TURN,
    UI_STATE_OPPONENT_TURN,
    UI_STATE_GAME_OVER
} UIState;

typedef struct {
    Board my_board;
    Board target_board;
    bool enemy_ships_sunk[NUM_SHIPS];
    PositionWithDirection enemy_ship_positions[NUM_SHIPS];
    bool i_won;
} GameState;

typedef struct {
    InitialShipDefs placements;
    int ships_placed;
    bool placing_horizontal;
} PlacementState;

typedef struct {
    int connection_fd;
    bool server_requested_board;
} NetworkState;

typedef struct {
    bool is_running; // while loop condition
    UIState current_state;
    GameState game;
    NetworkState net;
    PlacementState placement;
} ClientState;

extern ClientState init_client_state(void);
extern void free_client_state(ClientState *state);

#endif
