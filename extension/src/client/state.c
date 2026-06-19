#include "state.h"
#include "shared/board.h"

/*
Stores this player's board, opponents board, and current UI phase
*/

ClientState init_client_state(void) {
    ClientState state;

    state.is_running = false;

    state.current_state = UI_STATE_CONNECTING;

    state.game.my_board = create_empty_board();
    state.game.target_board = create_empty_board();
    for (int i = 0; i < NUM_SHIPS; i++) {
        state.game.enemy_ships_sunk[i] = false;
    }
    state.game.i_won = false;

    state.net.connection_fd = -1;
    state.net.server_requested_board = false;

    for (int i = 0; i < NUM_SHIPS; i++) {
        state.placement.placements[i].ship = 0;
        state.placement.placements[i].pwd.pos.x = 0;
        state.placement.placements[i].pwd.pos.y = 0;
        state.placement.placements[i].pwd.horizontal = true;
    }
    state.placement.ships_placed = 0;
    state.placement.placing_horizontal = true;
    return state;
}

void free_client_state(ClientState *state) {
    if (state->game.my_board != NULL) {
        free_board(state->game.my_board);
        state->game.my_board = NULL;
    }

    if (state->game.target_board != NULL) {
        free_board(state->game.target_board);
        state->game.target_board = NULL;
    }
}
