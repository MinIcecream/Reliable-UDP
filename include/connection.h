#ifndef CONNECTION_H
#define CONNECTION_H

#include <netinet/in.h>
#include <stdint.h>
#include <sys/socket.h>

#include "packet.h"

#define WINDOW_SIZE 5

typedef enum {
    SYN_SENT,
    SYN_RECEIVED,
    ESTABLISHED,
    FIN_SENT,
    CLOSED,
} connection_state_t;

typedef struct {
    uint32_t curr_seq; // Current sequence number to send to peer
    uint32_t initial_seq; // Initial generated sequence number.
    uint32_t next_expected; // Next expected byte to receive from peer
    connection_state_t state;
    uint32_t send_base; // Oldest unacknowledged seq number.
} connection_t;

void client_handle_timeout(connection_t *connection, int socket_id, struct sockaddr_in server_addr,
                         socklen_t server_addr_len);
void client_handle_state(connection_t *connection, tcp_packet_t packet, int socket_id,
                         struct sockaddr_in server_addr, socklen_t server_addr_len);
void server_handle_state(connection_t *connection, tcp_packet_t packet, int socket_id,
                         struct sockaddr_in client_addr, socklen_t client_addr_len,
                         char *buffer, size_t buffer_size);

#endif // CONNECTION_H
