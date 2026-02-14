#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include "common.h"
#include <stdlib.h>
#include <sys/select.h>

const char* MSG = "HELLO WORLD!";
const int BYTES_PER_PAYLOAD = 5;

// if timeout during handshake, close connection
// Else, resend last data packet
void handle_timeout(connection_t* connection) {
    switch (connection -> state) {
        case ESTABLISHED:
            printf("timeout during data send! Resending last packet, seq num %u\n", connection -> curr_seq);
            break;
        default:
            printf("Timeout during handshake!\n");
            connection -> state = CLOSED;
            break;
    }
}

// given a packet, switch on connection state. Update connection state, send packets as needed.
void handle_state(connection_t* connection, tcp_packet_t packet, int socket_id, struct sockaddr_in server_addr, socklen_t server_addr_len) {
    switch(connection -> state) {
        case SYN_SENT:
            //if server responds with SYN_ACK, send ACK and transition to ESTABLISHED.
            //else, close connection and exit.
            if ((packet.flags & (FLAG_SYN | FLAG_ACK)) == (FLAG_SYN | FLAG_ACK) && packet.ack == connection -> curr_seq) {
                printf("Received SYN_ACK!\n");
                connection -> next_expected = packet.seq_num + 1;
                connection -> state = ESTABLISHED;
                tcp_packet_t ack_packet;
                memset(&ack_packet, 0, sizeof(ack_packet));
                ack_packet.flags = FLAG_ACK;
                ack_packet.seq_num = connection -> curr_seq;
                ack_packet.ack = connection -> next_expected;
                int result = send_packet(ack_packet, socket_id, &server_addr, server_addr_len); //TODO: make this use serialization
                if (result != 0) {
                    fprintf(stderr, "Failed to send ACK packet\n");
                    return;
                }
                // Do NOT update curr_seq since ACK does not consume sequence number
                printf("Sent ACK packet with seq_num: %u and ack: %u\n", ack_packet.seq_num, ack_packet.ack);
                printf("Connection established!\n");

                // Connection established. Send first data packet.
                tcp_packet_t data_packet;
                memset(&data_packet, 0, sizeof(data_packet));
                data_packet.flags = FLAG_DAT;
                data_packet.seq_num = connection -> curr_seq;
                int start = connection -> curr_seq - connection -> initial_seq - 1;
                int data_len = bytes_to_send(MSG, start);
                // TODO: Fix paylaod to send data_len bytes
                memcpy(data_packet.payload, MSG + start, data_len);
                data_packet.payload_len = data_len;
                result = send_packet(data_packet, socket_id, &server_addr, server_addr_len);
                if (result != 0) {
                    fprintf(stderr, "Failed to send data packet\n");
                    return;
                }
                printf("sent data packet with seq_num: %u\n", data_packet.seq_num);
                connection -> curr_seq += data_len;

            }
            else {
                fprintf(stderr, "Failed to establish connection: expected SYN-ACK packet\n");
                return;
            }
            break;
        case ESTABLISHED:
            // if receive expected ack within timeout, send next packet.
            // Else, resend.
            printf("Received packet with seq_num: %u and ack: %u\n", packet.seq_num, packet.ack);
            printf("curr_seq: %u, next_expected: %u\n", connection -> curr_seq, connection -> next_expected);
            if ((packet.flags & FLAG_ACK) == FLAG_ACK && packet.ack == connection -> curr_seq) {
                printf("Received ACK for data packet!\n");
                // send next string.
                // Otherwise, if sent last string, close connection.
                if (connection -> curr_seq == connection->initial_seq + 1 + strlen(MSG)) {
                    printf("All packets fully ACK'd! Can close connection.\n");
                    connection -> state = CLOSED;
                }
                else {
                    // send packet. increment curr_seq, last_sent_index
                    printf("Sending next data!\n");
                    tcp_packet_t data_packet;
                    memset(&data_packet, 0, sizeof(data_packet));
                    data_packet.flags = FLAG_DAT;
                    data_packet.seq_num = connection -> curr_seq;
                    int start = connection -> curr_seq - connection -> initial_seq - 1;
                    int data_len = bytes_to_send(MSG, start);
                    // TODO: Fix paylaod to send data_len bytes
                    memcpy(data_packet.payload, MSG + start, data_len);
                    data_packet.payload_len = data_len;
                    int result = send_packet(data_packet, socket_id, &server_addr, server_addr_len);
                    if (result != 0) {
                        fprintf(stderr, "Failed to send data packet\n");
                        return;
                    }
                    printf("sent data packet with seq_num: %u\n", data_packet.seq_num);
                    connection -> curr_seq += data_len;
                }
            }
            else {
                fprintf(stderr, "Received invalid ack!\n");
            }
            break;
        default:
            // should not reach here
            break;
    }

}

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
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    socklen_t server_addr_len = sizeof(server_addr);

    // init connection
    connection_t connection; 
    connection.initial_seq = rand();
    connection.curr_seq = connection.initial_seq;
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

    char buffer[MAX_PAYLOAD_SIZE];
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(socket_id, &readfds);
    struct timeval tv;
    tv.tv_sec = TIMEOUT_SECONDS;
    tv.tv_usec = 0;

    while (connection.state != CLOSED) {

        int ret = select(socket_id + 1, &readfds, NULL, NULL, &tv);
        if (ret > 0) {
            int received_msg_len = recvfrom(socket_id, buffer, sizeof(buffer), 0, (struct sockaddr *)&server_addr, &server_addr_len);

            if (received_msg_len < 0) {
                perror("Receive failed");
                continue;
            }
            tcp_packet_t packet;
            deserialize_packet(buffer, &packet);
            handle_state(&connection, packet, socket_id, server_addr, server_addr_len);
        }
        else if (ret == 0) {
            handle_timeout(&connection);
        }
        else {
            perror("Select failed");
        }
      }
    return 0;    
}