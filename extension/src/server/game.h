#ifndef GAME_STATE_H
#define GAME_STATE_H

#include "shared/board.h"
#include <stdbool.h>
#include <unistd.h>

struct PlayerState {
    int socket_fd;
    Board board;
};

typedef struct PlayerState *PlayerState;

struct GameState;
typedef struct GameState *GameState;

extern GameState init_game_state(void);
extern void free_game_state(GameState state);
extern void close_connections(GameState state);
extern void setup_boards(GameState state);

extern bool populate_ships(GameState state, PlayerState player);
extern PlayerState new_player(int socket_fd);
extern void set_players(GameState state, PlayerState p1, PlayerState p2);
extern void play(GameState state);

#endif
