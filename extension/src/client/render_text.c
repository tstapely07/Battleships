#include "render.h"
#include <stdbool.h>
#include <stdio.h>

bool init_graphics(void) {
    printf("=== Initialising ===\n");
    return true;
}

bool is_window_open(void) { return true; }

static UIState prev_state = -1; // -1 forces a draw on the very first frame
static int prev_ships_placed = -1;

extern int get_staged_ship_count(void);
extern InitialShipState *get_staged_ships(void);

void render_frame(const ClientState *state) {
    int current_staged = get_staged_ship_count();
    if (state->current_state == prev_state &&
        current_staged == prev_ships_placed) {
        return;
    }

    prev_state = state->current_state;
    prev_ships_placed = current_staged;

    // printf("\033[H"); // clear terminal
    switch (state->current_state) {
    default:
    case UI_STATE_CONNECTING:
        printf("=== Connecting to server ===\n");
        break;
    case UI_STATE_PLACING_SHIPS: {
        printf("=== Place Your Ships ===\n");

        // 1. Get partial list from input
        int count = get_staged_ship_count();
        InitialShipState *staged = get_staged_ships();

        // Can't modify real board we have to clone it
        Board temp_board = create_empty_board();
        for (int i = 0; i < count; i++) {
            board_add_single_ship(staged[i], temp_board);
        }
        print_board(temp_board, stdout);
        free_board(temp_board);

        // Prompt the user for the next ship
        const char *ship_names[] = {"Battleship (Len 5)", "Carrier (Len 4)",
                                    "Cruiser (Len 3)", "Submarine (Len 3)",
                                    "Destroyer (Len 2)"};
        printf("\nCurrently placing: %s\n", ship_names[count]);
        printf("Enter 'x y h' for horizontal, or 'x y v' for vertical (e.g. 2 "
               "4 h):\n");
        break;
    }
    case UI_STATE_WAITING_FOR_OPPONENT:
        printf("=== Waiting for opponent ===\n");
        break;
    case UI_STATE_MY_TURN:
        printf("=== YOUR TURN ===\n");
        printf("=== Your board ===\n");
        print_board(state->game.my_board, stdout);
        printf("\n=== Target board ===\n");
        print_board(state->game.target_board, stdout);
        break;
    case UI_STATE_OPPONENT_TURN:
        printf("=== OPPONENT'S TURN ===\n");
        printf("=== Your board ===\n");
        print_board(state->game.my_board, stdout);
        printf("\n=== Target board ===\n");
        print_board(state->game.target_board, stdout);
        break;
    case UI_STATE_GAME_OVER:
        if (state->game.i_won) {
            printf("=== YOU WON!! ===\n");
        } else {
            printf("=== LOSER!! ==\n");
        }
        break;
    }
}

void cleanup_graphics(void) { return; }
