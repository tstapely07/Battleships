#include "client/game.h"
#include "shared/log.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/*
Entry point for client executable
Initialises local state storage and runs the game loop
*/

int main(int argc, char **argv) {
    // Defaults
    char *log_path = NULL;
    char *hostname = "127.0.0.1";
    int port = 8080;

    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        if ((strcmp(argv[i], "-l") == 0 || strcmp(argv[i], "--log") == 0) &&
            i + 1 < argc) {
            log_path = argv[i + 1];
            i++;
        } else if ((strcmp(argv[i], "-p") == 0 ||
                    strcmp(argv[i], "--port") == 0) &&
                   i + 1 < argc) {
            port = atoi(argv[i + 1]);
            i++;
        } else if ((strcmp(argv[i], "-h") == 0 ||
                    strcmp(argv[i], "--host") == 0) &&
                   i + 1 < argc) {
            hostname = argv[i + 1];
            i++;
        } else {
            printf("Usage: %s [-l | --log filepath] [-p | --port port] [ -h | "
                   "--host ip_address]\n",
                   argv[0]);
            return EXIT_FAILURE;
        }
    }

    // Setup logging
    if (!init_logger(log_path)) {
        return EXIT_FAILURE;
    }

    LOG_INFO("Client starting up. Target host: %s", hostname);

    // Start game
    ClientState state = init_client_state();
    state.is_running = true;

    if (start_client_systems(&state, hostname, port)) {
        client_loop(&state);
    }

    client_cleanup(&state);
    close_logger();

    return 0;
}
