#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "common.h"

int main() {
    int socket_id = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_id < 0) {
        perror("Socket creation failed");
        return 1;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(socket_id, (const struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        return 1;
    }

    const uint32_t WINDOW_SIZE = 1052;
    char buffer[WINDOW_SIZE];
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    connection_t connection;
    connection.curr_seq = rand();
    connection.state = CLOSED;

    while (1) {
        ssize_t received_msg_len = recvfrom(socket_id, buffer, sizeof(buffer), 0, (struct sockaddr *)&client_addr, &client_addr_len);
        if (received_msg_len < 0) {
            perror("Receive failed");
            continue;
        }

        tcp_packet_t packet = decode_packet(buffer);
        switch(connection.state) {
            case CLOSED:
                printf("flags: %u\n", packet.flags);
                if (packet.flags & FLAG_SYN) {
                    connection.expected_ack = packet.seq_num + 1;

                    tcp_packet_t syn_ack_packet;
                    memset(&syn_ack_packet, 0, sizeof(syn_ack_packet));
                    syn_ack_packet.flags = FLAG_SYN | FLAG_ACK;
                    syn_ack_packet.seq_num = htonl(connection.curr_seq);
                    syn_ack_packet.ack = htonl(connection.expected_ack);
                    int result = send_packet(syn_ack_packet, socket_id, &client_addr, client_addr_len);
                    if (result != 0) {
                        fprintf(stderr, "Failed to send SYN-ACK packet\n");
                        continue;
                    }
                    connection.state = SYN_RECEIVED;
                }
                break;
            case SYN_SENT:
                break;
            case SYN_RECEIVED:
                break;
            case ESTABLISHED:
                break;
        }
        printf("current state: %d\n", connection.state);
        printf("Received packet with seq_num: %u\n", packet.seq_num);
    
        printf("Received message: %.*s \n", (int)received_msg_len, buffer);
        int sent_msg_len = sendto(socket_id, buffer, received_msg_len, 0, (const struct sockaddr *)&client_addr, client_addr_len);
        printf("Echoed %d bytes back to client\n", sent_msg_len);
    }
    return 0;
}