#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "shared/protocol.h"
#include "shared/network.h"
#include "shared/log.h"
#include "client/network.h"

int main(int argc, char *argv[]) {
    init_logger(NULL);

    // Default to localhost if no argument is provided
    char *hostname = "127.0.0.1";
    if (argc > 1) {
        hostname = argv[1];
    }

    LOG_INFO("Test Client: Connecting to %s:8080...", hostname);

    int sockfd = connect_to_server(hostname, 8080);
    if (sockfd < 0) return EXIT_FAILURE;

    LOG_INFO("%s", "Test Client: Sending MSG_JOIN...");
    
    // Send a header-only packet
    if (send_packet(sockfd, MSG_JOIN, NULL, 0) < 0) {
        close(sockfd);
        return EXIT_FAILURE;
    }

    PacketHeader header;
    void *payload = NULL;
    
    LOG_INFO("%s", "Test Client: Waiting for response...");
    
    // Poll non-blocking socket for the server's reply
    int status;
    while ((status = receive_packet(sockfd, &header, &payload)) == 0) {
        usleep(10000); 
    }

    if (status == 1) {
        LOG_INFO("Test Client: Received response type %d", header.type);
        if (payload) free(payload);
    } else {
        LOG_ERROR("%s", "Test Client: Connection closed or failed.");
    }

    close(sockfd);
    close_logger();
    return EXIT_SUCCESS;
}