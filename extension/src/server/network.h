#ifndef SERVER_NETWORK_H
#define SERVER_NETWORK_H

int start_server(int port);

int accept_client(int server_fd);

#endif
