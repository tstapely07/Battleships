#include "game.h"
#include "shared/log.h"
#include "shared/network.h"
#include "shared/protocol.h"
#include <stdlib.h>
#include <time.h>

/*
Processes actions, overwrites grid cells, and checks for win conditions
*/

struct GameState {
    PlayerState player1;
    PlayerState player2;
    bool is_player1_turn;
};

/*
Creates an empty game with two null player states
*/
GameState init_game_state(void) {
    GameState state = malloc(sizeof(struct GameState));
    if (state == NULL) {
        return NULL;
    }

    srand(time(NULL));

    state->player1 = NULL;
    state->player2 = NULL;
    state->is_player1_turn = rand() > (RAND_MAX / 2);

    return state;
}

PlayerState new_player(int socket_fd) {
    PlayerState p = malloc(sizeof(struct PlayerState));
    if (p == NULL) {
        return NULL;
    }

    p->socket_fd = socket_fd;
    p->board = create_empty_board();
    return p;
}

void set_players(GameState state, PlayerState p1, PlayerState p2) {
    state->player1 = p1;
    state->player2 = p2;
}
/*
Frees all memory associated with the state. Ensure close_connections has already
been called
*/
void free_game_state(GameState state) {
    free(state->player1->board);
    free(state->player2->board);
    free(state->player1);
    free(state->player2);
    free(state);
}

/*
Destroys the conntections to both players
*/
void close_connections(GameState state) {
    close(state->player1->socket_fd);
    close(state->player2->socket_fd);
}

/*
Sends game over packets to bth players
*/
void end_game(GameState state, PlayerState winner, PlayerState loser) {
    GameOverPayload winnerData = {.you_won = true};
    GameOverPayload loserData = {.you_won = false};

    send_packet(winner->socket_fd, MSG_GAME_OVER, &winnerData,
                sizeof(GameOverPayload));
    send_packet(loser->socket_fd, MSG_GAME_OVER, &loserData,
                sizeof(GameOverPayload));
}

/*
Returns true if the ships have valid placements, otherwise return false
*/
bool populate_ships(GameState state, PlayerState player) {
    PacketHeader header;
    InitialShipDefs *defs = NULL;

    PlayerState other;
    if (player == state->player1) {
        other = state->player2;
    } else {
        other = state->player1;
    }

    // We wait until we get a response
    while (true) {
        int res = receive_packet(player->socket_fd, &header, (void **)&defs);
        if (res == 1 && header.type == MSG_INIT_BOARD_LAYOUT) {
            break; // Received packet
        }

        if (res == -1) {
            end_game(state, other, player);
            close_connections(state);
            free_game_state(state);
            exit(EXIT_FAILURE);
        }

        if (res == 1 && defs != NULL) {
            free(defs);
        }
    }

    bool res = board_add_placement_set(*defs, player->board);
    free(*defs);
    return res;
}

PlayerState get_player1(GameState state) { return state->player1; }
PlayerState get_player2(GameState state) { return state->player2; }

/*
Sends the packets declaring the start of the game with info of which player
starts
*/
static void send_start_packets(GameState state) {
    GameStartPayload your_turn = {.your_turn = true};
    GameStartPayload not_your_turn = {.your_turn = false};

    GameStartPayload *player1_payload;
    GameStartPayload *player2_payload;

    if (state->is_player1_turn) {
        player1_payload = &your_turn;
        player2_payload = &not_your_turn;
    } else {
        player1_payload = &not_your_turn;
        player2_payload = &your_turn;
    }

    send_packet(state->player1->socket_fd, MSG_GAME_START, player1_payload,
                sizeof(GameStartPayload));
    send_packet(state->player2->socket_fd, MSG_GAME_START, player2_payload,
                sizeof(GameStartPayload));
}

void populate_turn_players(GameState state, PlayerState *turn_taker,
                           PlayerState *other_player) {
    if (state->is_player1_turn) {
        *turn_taker = state->player1;
        *other_player = state->player2;
    } else {
        *turn_taker = state->player2;
        *other_player = state->player1;
    }
}

void play(GameState state) {
    // Keep requesting a ship placement until we get a valid one
    LOG_DEBUG("%s", "Requesting ship positions from the clients.");

    // Populate ships
    while (!populate_ships(state, state->player1)) {
        send_packet(state->player1->socket_fd, MSG_INVALID_BOARD, NULL, 0);
    }
    while (!populate_ships(state, state->player2)) {
        send_packet(state->player2->socket_fd, MSG_INVALID_BOARD, NULL, 0);
    }

    // Inform clients we are ready and inform them of whos turn it is
    send_start_packets(state);

    PlayerState turn_taker;
    PlayerState other_player;

    // Take turns
    do {
        populate_turn_players(state, &turn_taker, &other_player);
        PacketHeader header;
        FirePayload *fire_payload = NULL;
        int res = receive_packet(turn_taker->socket_fd, &header,
                                 (void **)&fire_payload);

        if (res == -1) {
            // Player disconnected or some other connection issue occurred on
            // their turn. They lose.
            free(fire_payload);
            end_game(state, other_player, turn_taker);
            close_connections(state);
            free_game_state(state);
            exit(EXIT_FAILURE);
        }
        // Received packet
        if (res != 1 || header.type != MSG_FIRE) {
            if (res == 1 && fire_payload != NULL) {
                free(fire_payload);
            }
            continue;
        }

        Position p = {.x = fire_payload->x, .y = fire_payload->y};

        bool was_hit = false;
        ShipType sunk = -1;

        if (!board_try_hit(other_player->board, p, &was_hit, &sunk)) {
            // A proper client will have ensured that this was a valid
            // position to attack. The player probably interfered wth
            // our networking. They lose.
            free(fire_payload);
            end_game(state, other_player, turn_taker);
            close_connections(state);
            free_game_state(state);
            exit(EXIT_FAILURE);
        }

        // Inform player of result
        HitResultPayload hrp = {.success = was_hit, .ship = sunk};

        // If a ship was sunk, send details
        if (sunk != -1) {
            ShipState sunk_state = board_get_ship(other_player->board, sunk);
            hrp.sunk_pwd = sunk_state.pwd;
        }

        EnemyAttackPayload eap = {.shot = *fire_payload, .result = hrp};
        free(fire_payload);

        send_packet(turn_taker->socket_fd, MSG_ATTACKED, &eap,
                    sizeof(EnemyAttackPayload));
        // Inform opponent of result
        send_packet(other_player->socket_fd, MSG_ATTACKED, &eap,
                    sizeof(EnemyAttackPayload));

        // SWAP TURNS
        state->is_player1_turn = !state->is_player1_turn;
    } while (!all_ships_destroyed(other_player->board));

    end_game(state, turn_taker, other_player);
}
