#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include "protocol.h"
#include "log.h"

/* Sends a packet header and optional payload to the server.
   Receives the connected socket file descriptor, the MessageType enum,
   a pointer to the payload being sent (which can be NULL) and the
   payload lenght in bytes.
   Returns 0 on successful transmission, -1 on failure. */
int send_packet(int sockfd, MessageType type, const void *payload,
                uint32_t payload_length) {
    PacketHeader header;
    header.type = type;

    // Convert the integer payload length to Network Byte Order before sending
    header.payload_length = htonl(payload_length);

    // Send the header
    int sent = send(sockfd, &header, sizeof(PacketHeader), MSG_DONTWAIT);
    if (sent < 0) {
        LOG_ERROR("%s", "Failed to send packet header");
        return -1;
    }

    // Send the payload, if there is one
    if (payload_length > 0 && payload != NULL) {
        sent = send(sockfd, payload, payload_length, 0);
        if (sent < 0) {
            LOG_ERROR("%s", "Failed to send packet payload");
            return -1;
        }
    }

    LOG_DEBUG("Sent packet (Type: %d, Payload Size: %u bytes)", type, payload_length);
    return 0;
}

/* Non-blocking poll to read incoming packets from the server. Designed to
   be called once per frame. If a payload is received, then the memory is
   dynamically allocated, and it is the caller's responsibility to free it.
   Receives the socket file descriptor, a pointer to a local PacketHeader to
   populate, and an address to a void pointer which will be set to newly
   allocated memory containing the payload (if any).
   Returns 1 if a packet was caught, 0 if nothing, and -1 on
   fatal error or disconnect. */

int receive_packet(int sockfd, PacketHeader *out_header, void **out_payload) {
    // Attempt to read the header
    int n = recv(sockfd, out_header, sizeof(PacketHeader), MSG_DONTWAIT);

    if (n < 0) {
        if (errno == EWOULDBLOCK || errno == EAGAIN) {
            // No data yet, but nothing went wrong
            return 0;
        }
        // If we got some other error, then that's a genuine error
        LOG_ERROR("%s", "Socket error during receive");
        return -1;
    } else if (n == 0) {
        LOG_ERROR("%s", "Connection was closed");
        return -1;
    }

    // Now we have a header
    out_header->payload_length = ntohl(out_header->payload_length);

    // Read the payload if we are expecting one
    if (out_header->payload_length > 0) {
        *out_payload = malloc(out_header->payload_length);

        if (*out_payload == NULL) {
            LOG_ERROR("%s", "Memory allocation failed for payload");
            return -1;
        }

        // Read the entire payload
        int total_read = 0;
        while (total_read < out_header->payload_length) {
            int payload_bytes =
                recv(sockfd, (char *)*out_payload + total_read,
                     out_header->payload_length - total_read, 0);
            if (payload_bytes > 0) {
                total_read += payload_bytes;
            } else if (payload_bytes == 0 || (payload_bytes < 0 && errno != EWOULDBLOCK && errno != EAGAIN)) {
                LOG_ERROR("%s", "Connection closed or failed while reading payload");
                free(*out_payload);
                *out_payload = NULL;
                return -1;
            }
        }
    } else {
        // Some packets have no payload
        *out_payload = NULL;
    }

    LOG_DEBUG("Received packet (Type: %d, Payload Size: %u bytes)", out_header->type, out_header->payload_length);
    return 1;
}
