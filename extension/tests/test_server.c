#include "server/network.h"
#include "shared/log.h"
#include "shared/network.h"
#include "shared/protocol.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main() {
    init_logger(NULL);

    int server_fd = start_server(8080);
    LOG_INFO("%s", "Test Server: Waiting for client...");

    int client_fd = accept_client(server_fd);
    if (client_fd < 0)
        return EXIT_FAILURE;

    PacketHeader header;
    void *payload = NULL;

    LOG_INFO("%s", "Test Server: Waiting for packet...");

    // Poll non-blocking socket
    int status;
    while ((status = receive_packet(client_fd, &header, &payload)) == 0) {
        usleep(10000); // Sleep to prevent 100% CPU
    }

    if (status == 1) {
        LOG_INFO("Test Server: Received packet type %d", header.type);
        if (payload)
            free(payload);

        // Send a response
        GameStartPayload response = {.your_turn = true};
        send_packet(client_fd, MSG_GAME_START, &response,
                    sizeof(GameStartPayload));

        close(client_fd);
        close(server_fd);
    } else {
        LOG_ERROR("%s", "Test Server: Connection closed or failed.");
    }

    close_logger();
    return EXIT_SUCCESS;
}