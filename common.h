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
    uint32_t seq_num;
    uint32_t ack;
    char payload[MAX_PAYLOAD_SIZE];
    uint16_t payload_len;
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

#endif // COMMON_H