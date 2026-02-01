#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include "common.h"
#include <stdlib.h>

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

    // Initiate connection
    tcp_packet_t syn_packet;
    memset(&syn_packet, 0, sizeof(syn_packet));
    syn_packet.flags = FLAG_SYN;
    syn_packet.seq_num = connection.curr_seq;
    int result = send_packet(syn_packet, socket_id, &server_addr, server_addr_len);
    if (result != 0) {
        fprintf(stderr, "Failed to send SYN packet\n");
        return 1;
    }
    connection.state = SYN_SENT;
    printf("Sent SYN packet with seq_num: %u\n", connection.curr_seq);
    printf("current state: %d\n", connection.state);
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