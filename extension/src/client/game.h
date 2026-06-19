#ifndef GAME_H
#define GAME_H

#include "state.h"

extern bool start_client_systems(ClientState *state, char *hostname, int port);

extern void client_loop(ClientState *state);

extern void client_cleanup(ClientState *state);

#endif
