#include "server/game.h"
#include "server/network.h"
#include "shared/network.h"
#include "shared/protocol.h"
#include "shared/log.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

/*
Server executable:
    Connects to players, and boots up the state machine
*/

int main(int argc, char **argv) {
    // Defaults
    char *log_path = NULL;
    int port = 8080;

    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        if ((strcmp(argv[i], "-l") == 0 || strcmp(argv[i], "--log") == 0) && i + 1 < argc) {
            log_path = argv[i + 1];
            i++;
        }
        else if ((strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--port") == 0) && i + 1 < argc) {
            port = atoi(argv[i + 1]);
            i++;
        } else {
            printf("Usage: %s [-l | --log filepath] [-p | --port port]\n", argv[0]);
            return EXIT_FAILURE;
        }
    }

    // Setup logging
    if (!init_logger(log_path)) {
        return EXIT_FAILURE;
    }

    // Start server
    int server_fd = start_server(port);

    LOG_INFO("%s", "Waiting for Player 1...");
    int player1_fd = accept_client(server_fd);
    LOG_INFO("%s", "Waiting for Player 2...");
    int player2_fd = accept_client(server_fd);

    if (player1_fd == -1) {
        LOG_ERROR("%s", "Player 1 failed to connect!");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    LOG_INFO("%s", "Player 1 connected successfully!");

    if (player2_fd == -1) {
        LOG_ERROR("%s", "Player 2 failed to connect!");
        close(player1_fd);
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Set up server state
    GameState state = init_game_state();

    if (state == NULL) {
        close(player1_fd);
        close(server_fd);
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    send_packet(player1_fd, MSG_REQ_BOARD, NULL, 0);
    send_packet(player2_fd, MSG_REQ_BOARD, NULL, 0);

    // Set up players
    PlayerState p1 = new_player(player1_fd);
    PlayerState p2 = new_player(player2_fd);
    set_players(state, p1, p2);

    play(state);

    // Cleanup
    close_connections(state);
    free_game_state(state);

    close(server_fd);

    LOG_INFO("%s", "Server shutting down.");
    close_logger();

    return 0;
}
