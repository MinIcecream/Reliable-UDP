#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include "common.h"
#include <stdlib.h>

// Sends initial handshake message and updates connection state.
// Returns 0 on success, 1 on failure.
int initiate_connection(int socket_id, struct sockaddr_in *server_addr, socklen_t server_addr_len, connection_t connection) {
    tcp_packet_t syn_packet;
    memset(&syn_packet, 0, sizeof(syn_packet));
    syn_packet.flags = FLAG_SYN;
    syn_packet.seq_num = htonl(connection.curr_seq);
    printf("Sending SYN with seq_num: %u\n", ntohl(syn_packet.seq_num));
    ssize_t sent_bytes = sendto(socket_id, &syn_packet, sizeof(syn_packet), 0, (const struct sockaddr *)server_addr, server_addr_len);
    if (sent_bytes < 0) {
        perror("Failed to send SYN packet");
        return 1;
    }
    connection.state = SYN_SENT;
    return 0;

}

int main() {
    int socket_id = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_id < 0) {
        perror("Socket creation failed");
        return 1;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    socklen_t server_addr_len = sizeof(server_addr);

    connection_t connection;
    connection.curr_seq = rand();
    connection.state = CLOSED;

    int result = initiate_connection(socket_id, &server_addr, server_addr_len, connection);
    if (result != 0) {
        fprintf(stderr, "Failed to initiate connection\n");
        return 1;
    }
    // char *message = "Hello, Server!";
    // int sent_msg_len = sendto(socket_id, message, strlen(message), 0, (const struct sockaddr *)&server_addr, sizeof(server_addr));
    // if (sent_msg_len < 0) {
    //     perror("Send failed");
    //     return 1;
    // }
    // printf("sent %d bytes! \n", sent_msg_len);
    // char buffer[1024];
    // int received_msg_len = recvfrom(socket_id, buffer, sizeof(buffer), 0, (struct sockaddr *)&server_addr, &server_addr_len);
    // printf("Received message back from server: %.*s \n", received_msg_len, buffer);
    // return 0;
}