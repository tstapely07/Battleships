#include "client/input.h"
#include "client/network.h"
#include "client/render.h"
#include "client/state.h"
#include "shared/log.h"
#include "shared/network.h"
#include "shared/protocol.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// Returns true on success, false on failure
bool start_client_systems(ClientState *state, char *hostname, int port) {
    if (!init_graphics()) {
        LOG_ERROR("%s", "Failed to initialise graphics engine");
        return false;
    }

    LOG_INFO("%s", "Attempting to connect to server...");
    state->net.connection_fd = connect_to_server(hostname, port);

    if (state->net.connection_fd != -1) {
        LOG_INFO("%s", "Connected successfully!");
        state->current_state = UI_STATE_PLACING_SHIPS;
        return true;
    } else {
        LOG_ERROR("%s", "Failed to connect to server!");
        return false;
    }
}

static void send_board_to_server(ClientState *state) {
    InitBoardPayload payload;
    for (int i = 0; i < NUM_SHIPS; i++) {
        payload.defs[i] = state->placement.placements[i];
    }
    send_packet(state->net.connection_fd, MSG_INIT_BOARD_LAYOUT, &payload,
                sizeof(payload));
    LOG_DEBUG("%s", "Sent board layout to server.");
}

static void handle_msq_req_board(ClientState *state) {
    state->net.server_requested_board = true;
    if (state->current_state == UI_STATE_WAITING_FOR_OPPONENT) {
        send_board_to_server(state);
        LOG_DEBUG("%s", "Sent ship layout to server.");
    } else {
        LOG_DEBUG("%s",
                  "Server requested board, but we are still placing ships.");
    }
}

static void handle_msg_game_start(ClientState *state,
                                  GameStartPayload *start_data) {
    state->current_state =
        start_data->your_turn ? UI_STATE_MY_TURN : UI_STATE_OPPONENT_TURN;
    LOG_INFO("%s", "Game has been started.");
}

static void handle_msg_attack(ClientState *state,
                              EnemyAttackPayload *hit_data) {
    FirePayload shot = hit_data->shot;
    HitResultPayload result = hit_data->result;

    Board target = (state->current_state == UI_STATE_MY_TURN)
                       ? state->game.target_board
                       : state->game.my_board;
    UIState next_state = (state->current_state == UI_STATE_MY_TURN)
                             ? UI_STATE_OPPONENT_TURN
                             : UI_STATE_MY_TURN;

    board_mark_strike(target, (Position){.x = shot.x, .y = shot.y},
                      result.success);

    // Now check if the shot sank a ship
    if (result.ship != -1) {
        board_mark_sunk_ship(target, result.ship, result.sunk_pwd);
        if (state->current_state == UI_STATE_MY_TURN) {
            state->game.enemy_ships_sunk[result.ship] = true;
            state->game.enemy_ship_positions[result.ship] = result.sunk_pwd;
        }

        LOG_INFO("%s", "Ship sunk!");
    } else {
        LOG_DEBUG("%s", "Marked the hit.");
    }

    state->current_state = next_state;
}

static void handle_msg_game_over(ClientState *state,
                                 GameOverPayload *result_data) {
    state->game.i_won = result_data->you_won;

    state->current_state = UI_STATE_GAME_OVER;

    char *winner = state->game.i_won ? "You" : "Your opponent";
    LOG_INFO("%s won!", winner);
}

static void handle_incoming_packet(ClientState *state, PacketHeader header,
                                   void *payload) {
    switch (header.type) {
    case MSG_REQ_BOARD:
        handle_msq_req_board(state);
        break;
    case MSG_GAME_START:
        handle_msg_game_start(state, (GameStartPayload *)payload);
        break;
    case MSG_ATTACKED:
        handle_msg_attack(state, (EnemyAttackPayload *)payload);
        break;
    case MSG_GAME_OVER:
        // This is also called unexpectedly in the case where the opponent
        // resigned or similar
        handle_msg_game_over(state, (GameOverPayload *)payload);
        break;
    case MSG_INVALID_BOARD:
        LOG_ERROR("%s",
                  "Invalid placement: ships overlap or are out of bounds!");

        reset_staged_ships();

        free_board(state->game.my_board);
        state->game.my_board = create_empty_board();
        break;
    default:
        LOG_ERROR("%s", "Unexpected packet type received");
    }
}

static void handle_state_placing_ships(ClientState *state, InputData input) {
    // Attempt to place
    if (input.type != INPUT_PLACED_SHIPS) {
        return;
    }

    if (board_add_placement_set(input.ships, state->game.my_board)) {
        // Success
        for (int i = 0; i < NUM_SHIPS; i++) {
            state->placement.placements[i] = input.ships[i];
        }

        state->current_state = UI_STATE_WAITING_FOR_OPPONENT;
        LOG_INFO("%s", "Ships placed successfully! Waiting for opponent...");

        if (state->net.server_requested_board) {
            send_board_to_server(state);
        }
    } else {
        // Invalid!
        LOG_ERROR("%s",
                  "Invalid placement: ships overlap or are out of bounds!");

        reset_staged_ships();

        free_board(state->game.my_board);
        state->game.my_board = create_empty_board();
    }
}

static void handle_state_my_turn(ClientState *state, InputData input) {
    // Check if we got an input
    if (input.type != INPUT_FIRE) {
        return;
    }
    if (valid_attack_pos(state->game.target_board, input.grid_pos)) {
        FirePayload fire_req;
        fire_req.x = input.grid_pos.x;
        fire_req.y = input.grid_pos.y;

        send_packet(state->net.connection_fd, MSG_FIRE, &fire_req,
                    sizeof(fire_req));
        LOG_DEBUG("Fired at %d, %d!", input.grid_pos.x, input.grid_pos.y);
    } else {
        LOG_DEBUG("%s", "Invalid coordinate!");
    }
}

void client_loop(ClientState *state) {
    while (state->is_running && is_window_open()) {
        // Server shuts down after game over, so we must check this to avoid
        // exiting
        if (state->current_state != UI_STATE_GAME_OVER) {
            // Receive data from server
            PacketHeader header;
            void *payload = NULL;
            int recv_status;
            while ((recv_status = receive_packet(state->net.connection_fd,
                                                 &header, &payload)) == 1) {
                handle_incoming_packet(state, header, payload);
                if (payload) {
                    free(payload);
                }
            }

            // Check for crash/disconnect
            if (recv_status == -1 &&
                state->current_state != UI_STATE_GAME_OVER) {
                LOG_ERROR("%s", "Lost connection to the server! Exiting...");
                state->is_running = false;
                break; // Break out of the game loop immediately
            }
        }

        // Receive input
        InputData input = get_user_input();
        if (input.type == INPUT_QUIT) {
            state->is_running = false;
        }

        switch (state->current_state) {
        case UI_STATE_PLACING_SHIPS:
            handle_state_placing_ships(state, input);
            break;
        case UI_STATE_MY_TURN:
            handle_state_my_turn(state, input);
            break;
        case UI_STATE_GAME_OVER:
        case UI_STATE_OPPONENT_TURN:
        case UI_STATE_WAITING_FOR_OPPONENT:
        case UI_STATE_CONNECTING:
            // Do nothing, just sit tight
            break;
        }

        render_frame(state);
    }
}

void client_cleanup(ClientState *state) {
    if (state->net.connection_fd != -1) {
        close(state->net.connection_fd);
    }

    free_client_state(state);
    cleanup_graphics();
    LOG_INFO("%s", "Client shut down cleanly.");
}
