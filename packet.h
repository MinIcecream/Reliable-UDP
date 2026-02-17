#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>

#define MAX_PAYLOAD_SIZE 5
#define MAX_PACKET_SIZE (14 + MAX_PAYLOAD_SIZE) // 14 bytes for header

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

void serialize_packet(tcp_packet_t packet, char *buffer);
void deserialize_packet(char *buffer, tcp_packet_t *packet);

#endif // PROTOCOL_H
