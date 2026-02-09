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
    printf("Sent SYN packet with seq_num: %u\n", connection.curr_seq);
    connection.curr_seq += 1;
    connection.state = SYN_SENT;

    while (1) {
        char buffer[MAX_PAYLOAD_SIZE];
        int received_msg_len = recvfrom(socket_id, buffer, sizeof(buffer), 0, (struct sockaddr *)&server_addr, &server_addr_len);

        if (received_msg_len < 0) {
            perror("Receive failed");
            continue;
        }
        tcp_packet_t packet;
        deserialize_packet(buffer, &packet);
        
        switch(connection.state) {
            case SYN_SENT:
                //if server responds with SYN_ACK, send ACK and transition to ESTABLISHED.
                //else, close connection and exit.
                if ((packet.flags & (FLAG_SYN | FLAG_ACK)) == (FLAG_SYN | FLAG_ACK) && packet.ack == connection.curr_seq) {
                    printf("Received SYN_ACK!\n");
                    connection.next_expected = packet.seq_num + 1;
                    connection.state = ESTABLISHED;
                    tcp_packet_t ack_packet;
                    memset(&ack_packet, 0, sizeof(ack_packet));
                    ack_packet.flags = FLAG_ACK;
                    ack_packet.seq_num = connection.curr_seq;
                    ack_packet.ack = connection.next_expected;
                    serialize_packet(ack_packet, buffer);
                    int result = send_packet(ack_packet, socket_id, &server_addr, server_addr_len);
                    if (result != 0) {
                        fprintf(stderr, "Failed to send ACK packet\n");
                        continue;
                    }
                    // Do NOT update curr_seq since ACK does not consume sequence number
                    printf("Sent ACK packet with seq_num: %u and ack: %u\n", ack_packet.seq_num, ack_packet.ack);
                    printf("Connection established!\n");

                    // Connection established. Send first data packet.
                    tcp_packet_t data_packet;
                    memset(&data_packet, 0, sizeof(data_packet));
                    data_packet.flags = FLAG_DAT;
                    data_packet.seq_num = connection.curr_seq;
                    strcpy(data_packet.payload, "Hello server!");
                    data_packet.payload_len = strlen(data_packet.payload);
                    serialize_packet(data_packet, buffer);
                    result = send_packet(data_packet, socket_id, &server_addr, server_addr_len);
                    if (result != 0) {
                        fprintf(stderr, "Failed to send data packet\n");
                        continue;
                    }
                    printf("sent data packet with seq_num: %u\n", data_packet.seq_num);
                    connection.curr_seq += data_packet.payload_len;

                }
                else {
                    fprintf(stderr, "Failed to establish connection: expected SYN-ACK packet\n");
                    return 1;
                }
                break;
            case ESTABLISHED:
                printf("Received packet with seq_num: %u and ack: %u\n", packet.seq_num, packet.ack);
                printf("curr_seq: %u, next_expected: %u\n", connection.curr_seq, connection.next_expected);
                if ((packet.flags & FLAG_ACK) == FLAG_ACK && packet.ack == connection.curr_seq) {
                    printf("Received ACK for data packet!\n");
                }
                else {
                    fprintf(stderr, "Failed to receive ACK for data packet\n");
                }
                // if ACK not in window(previously sent ACK + window size), close connection and exit.
                // send packets starting at ACK
                break;
            default:
                // should not reach here
                break;
        }
    }
    return 0;    
}