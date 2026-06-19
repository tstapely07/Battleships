#include "shared/log.h"
#include "shared/protocol.h"
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

/* Starts the server, binds to the specified port, and begins listening.
   Exits if a failure occurs.
   Returns the server socket file descriptor. */
int start_server(int port) {
    int server_fd;
    struct sockaddr_in server_addr;

    // Create the socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        LOG_ERROR("%s", "Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Prevents address already in use errors during testing.
    // After an exit, the port is usually held for a few minutes,
    // so we must override that.
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        LOG_ERROR("%s", "setsockopt failed");
        exit(EXIT_FAILURE);
    }

    // Configure server address struct
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY; // Listen everywhere
    server_addr.sin_port = htons(port);

    // Bind socket to port
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) <
        0) {
        LOG_ERROR("%s", "Bind failed");
        exit(EXIT_FAILURE);
    }

    // Get the hostname where the server is running
    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) == -1) {
        // Fallback just in case gethostname fails
        strncpy(hostname, "unknown-host", sizeof(hostname));
    }

    // Listen for incoming connections, with a max queue length of 2
    if (listen(server_fd, 2) < 0) {
        LOG_ERROR("%s", "Listen failed");
        exit(EXIT_FAILURE);
    }

    LOG_INFO("Server %s listening on port %d", hostname, port);
    return server_fd;
}

/* BLocks until a client connects.
   Then returns the client socket file descriptor. */
int accept_client(int server_fd) {
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    int client_fd =
        accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
    if (client_fd < 0) {
        LOG_ERROR("%s", "Failed to accept client");
        return -1;
    }

    LOG_INFO("Client connected from %s:%d", inet_ntoa(client_addr.sin_addr),
             ntohs(client_addr.sin_port));

    // Set newly accepted client socket to non-blocking
    int flags = fcntl(client_fd, F_GETFL, 0);
    fcntl(client_fd, F_SETFL, flags | O_NONBLOCK);

    return client_fd;
}
