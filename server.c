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

        tcp_packet_t packet;
        deserialize_packet(buffer, &packet);
        switch(connection.state) {
            case CLOSED:
                // if packet is SYN, respond with SYN_ACK and transition to SYN_RECEIVED.
                if (packet.flags & FLAG_SYN) {
                    connection.next_expected = packet.seq_num + 1;

                    tcp_packet_t syn_ack_packet;
                    memset(&syn_ack_packet, 0, sizeof(syn_ack_packet));
                    syn_ack_packet.flags = FLAG_SYN | FLAG_ACK;
                    syn_ack_packet.seq_num = connection.curr_seq;
                    syn_ack_packet.ack = connection.next_expected;
                    serialize_packet(syn_ack_packet, buffer);
                    int result = send_packet(syn_ack_packet, socket_id, &client_addr, client_addr_len);
                    if (result != 0) {
                        fprintf(stderr, "Failed to send SYN-ACK packet\n");
                        continue;
                    }
                    connection.state = SYN_RECEIVED;
                    printf("Sent SYN-ACK packet with seq_num: %u and ack: %u\n", syn_ack_packet.seq_num, syn_ack_packet.ack);
                }
                break;
            case SYN_RECEIVED:
                // if client responds with ACK and ACK is correct, transition to ESTABLISHED.
                // else, close connection and exit.
                if ((packet.flags & FLAG_ACK) == FLAG_ACK && packet.ack == connection.curr_seq + 1) {
                    printf("Received ACK, connection established!\n");
                    connection.state = ESTABLISHED;
                }
                else {
                    fprintf(stderr, "Failed to establish connection, expected ACK with ack_num: %u\n", connection.curr_seq + 1);
                    connection.state = CLOSED;
                }
                break;
            case ESTABLISHED:
                // if packet seq_number not expected, resend previous ack.
                // else, send ACK for packet and process payload.
                break;
            default:
            // should not reach here
                break;
        }
    }
    return 0;
}