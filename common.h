#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifndef COMMON_H
#define COMMON_H

#define PORT 9001
#define MAX_PAYLOAD_SIZE 1024

#define FLAG_ACK 0x1
#define FLAG_SYN 0x2
#define FLAG_DAT 0x4
#define FLAG_FIN 0x8

typedef struct {
    uint32_t flags;
    uint32_t seq_num;
    uint32_t ack;
    uint16_t payload_len;
    char payload[MAX_PAYLOAD_SIZE];
} tcp_packet_t;

typedef enum {
    SYN_SENT,
    SYN_RECEIVED,
    ESTABLISHED,
    CLOSED,
} connection_state_t;

typedef struct {
    uint32_t curr_seq; // Current sequence number to send to peer
    uint32_t expected_ack; // Next expected acknowledgment number from peer
    connection_state_t state;
} connection_t;

void serialize_packet(tcp_packet_t packet, char *buffer) {
    *(uint32_t *)buffer = htonl(packet.flags);
    *(uint32_t *)(buffer + 4) = htonl(packet.seq_num);
    *(uint32_t *)(buffer + 8) = htonl(packet.ack);
    *(uint16_t *)(buffer + 12) = htons(packet.payload_len);
    memcpy(buffer + 14, packet.payload, packet.payload_len);
}

void deserialize_packet(char * buffer, tcp_packet_t* packet) {
    packet->flags = ntohl(*(uint32_t *)buffer);
    packet->seq_num = ntohl(*(uint32_t *)(buffer + 4));
    packet->ack = ntohl(*(uint32_t *)(buffer + 8));
    packet->payload_len = ntohs(*(uint16_t *)(buffer + 12));
    memcpy(packet->payload, buffer + 14, packet->payload_len);
}

int send_packet(tcp_packet_t packet, int socket_id, struct sockaddr_in *destination_addr, socklen_t destination_addr_len) {
    packet.flags = htonl(packet.flags);
    packet.seq_num = htonl(packet.seq_num);
    packet.ack = htonl(packet.ack);
    packet.payload_len = htons(packet.payload_len);

    ssize_t sent_bytes = sendto(socket_id, &packet, sizeof(packet), 0, (const struct sockaddr *)destination_addr, destination_addr_len);
    if (sent_bytes < 0) {
        perror("Failed to send SYN packet");
        return 1;
    }
    return 0;
}


#endif // COMMON_H