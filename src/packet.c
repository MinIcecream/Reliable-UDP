#include "packet.h"
#include <stdio.h>
#include <arpa/inet.h>
#include <string.h>
#include "logger.h"

void serialize_packet(tcp_packet_t packet, char *buffer) {
    *(uint32_t *)buffer = htonl(packet.flags);
    *(uint32_t *)(buffer + 4) = htonl(packet.seq_num);
    *(uint32_t *)(buffer + 8) = htonl(packet.ack);
    *(uint16_t *)(buffer + 12) = htons(packet.payload_len);
    memcpy(buffer + 14, packet.payload, packet.payload_len);
}

void deserialize_packet(char *buffer, tcp_packet_t *packet) {
    packet->flags = ntohl(*(uint32_t *)buffer);
    packet->seq_num = ntohl(*(uint32_t *)(buffer + 4));
    packet->ack = ntohl(*(uint32_t *)(buffer + 8));
    packet->payload_len = ntohs(*(uint16_t *)(buffer + 12));
    memcpy(packet->payload, buffer + 14, packet->payload_len);
}

#define N_BUFFERS 4
#define BUFFER_SIZE 256

char* to_string(tcp_packet_t packet) {
    static char buffers[N_BUFFERS][BUFFER_SIZE];
    static int index = 0;

    char* buffer = buffers[index];
    index = (index + 1) % N_BUFFERS;

    snprintf(buffer, BUFFER_SIZE,
        "Packet - Flags: 0x%x, Seq: %u, Ack: %u, Payload Len: %u, Payload: %.*s",
        packet.flags, packet.seq_num, packet.ack, packet.payload_len,
        packet.payload_len, packet.payload);

    return buffer;
}