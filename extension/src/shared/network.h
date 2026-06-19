#ifndef SHARED_NETWORK_H
#define SHARED_NETWORK_H

#include "protocol.h"

extern int send_packet(int sockfd, MessageType type, const void *payload,
                       uint32_t payload_length);

int receive_packet(int sockfd, PacketHeader *out_header, void **out_payload);

#endif
