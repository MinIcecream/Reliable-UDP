#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "common.h"
#include "connection.h"
#include "transport.h"
#include <stdlib.h>

int main() {
    int socket_id = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_id < 0) {
        perror("Socket creation failed");
        return 1;
    }

    // init socket
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // bind socket
    if (bind(socket_id, (const struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        return 1;
    }

    // init buffer and client address
    char buffer[MAX_PACKET_SIZE];
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    // init connection
    connection_t connection;
    connection.state = CLOSED;
    connection.initial_seq = rand();
    connection.curr_seq = connection.initial_seq;

    while (1) {
        ssize_t received_msg_len = recvfrom(socket_id, buffer, sizeof(buffer), 0, (struct sockaddr *)&client_addr, &client_addr_len);
        if (received_msg_len < 0) {
            perror("Receive failed");
            continue;
        }

        tcp_packet_t packet;
        deserialize_packet(buffer, &packet);
        server_handle_state(&connection, packet, socket_id, client_addr, client_addr_len, buffer, sizeof(buffer));
    }
    return 0;
}