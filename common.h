#include <stdint.h>
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
    uint32_t ack;
    uint32_t seq_num;
    char payload[MAX_PAYLOAD_SIZE];
    uint16_t payload_len;
} tcp_packet_t;

#endif // COMMON_H