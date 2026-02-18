#include "connection.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "transport.h"
#include "logger.h"

static const char *kMsg = "HELLO WORLD!";

static void packet_init(tcp_packet_t *packet, uint32_t flags, uint32_t seq_num, uint32_t ack) {
    memset(packet, 0, sizeof(*packet));
    packet->flags = flags;
    packet->seq_num = seq_num;
    packet->ack = ack;
}


static int send_ack_packet(int socket_id, struct sockaddr_in *addr, socklen_t addr_len,
                           uint32_t seq_num, uint32_t ack_num) {
    tcp_packet_t ack_packet;
    packet_init(&ack_packet, FLAG_ACK, seq_num, ack_num);
    return send_packet(ack_packet, socket_id, addr, addr_len);
}

// Given a message and starting at start_index, return number of bytes to send.
static int bytes_to_send(const char *message, int start_index) {
    int bytes = MAX_PAYLOAD_SIZE;
    if (start_index + bytes - 1 >= (int)strlen(message)) {
        bytes = (int)strlen(message) - start_index;
    }
    return bytes;
}

void client_send_packets(connection_t *connection, int socket_id, struct sockaddr_in server_addr,
                         socklen_t server_addr_len) {

    // connection -> send_base is the oldest unacknowledged seq num. curr_seq is the next seq num to send.
    // while bytes_to_send > 0 and curr_seq < send_base + WINDOW_SIZE * MAX_PAYLOAD_SIZE, send packet and increment curr_seq by bytes sent.
    int data_len = bytes_to_send(kMsg, (int)(connection->curr_seq - connection->initial_seq - 1));
    while (data_len > 0 && connection->curr_seq < connection->send_base + WINDOW_SIZE * MAX_PAYLOAD_SIZE) {
        tcp_packet_t data_packet;
        packet_init(&data_packet, FLAG_DAT, connection->curr_seq, 0);
        memcpy(data_packet.payload, kMsg + (int)(connection->curr_seq - connection->initial_seq - 1), data_len);
        data_packet.payload_len = data_len;
        int result = send_packet(data_packet, socket_id, &server_addr, server_addr_len);
        if (result != 0) {
            return;
        }
        connection -> curr_seq += data_len;
        data_len = bytes_to_send(kMsg, (int)(connection->curr_seq - connection->initial_seq - 1));
    }
}

void client_handle_timeout(connection_t *connection, int socket_id, struct sockaddr_in server_addr,
                         socklen_t server_addr_len) {
    switch (connection->state) {
        case ESTABLISHED:
            connection->curr_seq = connection->send_base + 1;
            client_send_packets(connection, socket_id, server_addr, server_addr_len);
            LOG_WARN("Timeout during data send, resending packets");
            break;
        case FIN_SENT:
            LOG_WARN("Timeout during connection termination, resending FIN packet");
            tcp_packet_t fin_packet;
            packet_init(&fin_packet, FLAG_FIN, connection->curr_seq, 0);
            int result = send_packet(fin_packet, socket_id, &server_addr, server_addr_len);
            if (result != 0) {
                return;
            }
            break;
        default:
            LOG_ERROR("Timeout during handshake, closing");
            connection->state = CLOSED;
            break;
    }
}

void client_handle_state(connection_t *connection, tcp_packet_t packet, int socket_id,
                         struct sockaddr_in server_addr, socklen_t server_addr_len) {
    switch (connection->state) {
        case SYN_SENT:
            // if server responds with SYN_ACK, send ACK and transition to ESTABLISHED.
            // else, close connection and exit.
            if ((packet.flags & (FLAG_SYN | FLAG_ACK)) == (FLAG_SYN | FLAG_ACK) &&
                packet.ack == connection->curr_seq) {
                connection->next_expected = packet.seq_num + 1;
                connection->state = ESTABLISHED;
                int result = send_ack_packet(socket_id, &server_addr, server_addr_len,
                                             connection->curr_seq, connection->next_expected);
                if (result != 0) {
                    return;
                }
                // Do NOT update curr_seq since ACK does not consume sequence number
                // Connection established. Send first data packet.
                client_send_packets(connection, socket_id, server_addr, server_addr_len);
            } else {
                LOG_ERROR("Failed to establish connection: expected SYN-ACK packet");
                return;
            }
            break;
        case ESTABLISHED:
            if ((packet.flags & FLAG_ACK) == FLAG_ACK && packet.ack > connection->send_base && packet.ack <= connection->curr_seq) {
                connection->send_base = packet.ack - 1;
                // If sent last string, close connection.
                if (connection->send_base == connection->initial_seq + strlen(kMsg)) {
                    LOG_INFO("All packets fully ACK'd, sending FIN");
                    tcp_packet_t fin_packet;
                    packet_init(&fin_packet, FLAG_FIN, connection->curr_seq, 0);
                    int result = send_packet(fin_packet, socket_id, &server_addr, server_addr_len);
                    if (result != 0) {
                        return;
                    }
                    connection->state = FIN_SENT;
                    connection->curr_seq += 1;
                } else {
                    // send packet. increment curr_seq, last_sent_index
                    client_send_packets(connection, socket_id, server_addr, server_addr_len);
                }
            } else {
                LOG_WARN("Received invalid ack");
            }
            break;
        case FIN_SENT:
            // if receive ACK for FIN, transition to CLOSED. Else, resend FIN.
            if ((packet.flags & FLAG_ACK) == FLAG_ACK && packet.ack == connection->curr_seq) {
                LOG_INFO("Received ACK for FIN, connection closed");
                connection->state = CLOSED;
            } else {
                LOG_WARN("Expected ACK for FIN");
            }
            break;
        default:
            // should not reach here
            break;
    }
}

void server_handle_state(connection_t *connection, tcp_packet_t packet, int socket_id,
                         struct sockaddr_in client_addr, socklen_t client_addr_len,
                         char *buffer, size_t buffer_size) {
    (void)buffer_size;
    switch (connection->state) {
        case CLOSED:
            // if packet is SYN, respond with SYN_ACK and transition to SYN_RECEIVED.
            if (packet.flags & FLAG_SYN) {
                connection->next_expected = packet.seq_num + 1;

                tcp_packet_t syn_ack_packet;
                packet_init(&syn_ack_packet, FLAG_SYN | FLAG_ACK, connection->curr_seq,
                    connection->next_expected);
                serialize_packet(syn_ack_packet, buffer);
                int result = send_packet(syn_ack_packet, socket_id, &client_addr, client_addr_len);
                if (result != 0) {
                    return;
                }
                connection->state = SYN_RECEIVED;
            }
            break;
        case SYN_RECEIVED:
            // if client responds with ACK and ACK is correct, transition to ESTABLISHED.
            // else, close connection and exit.
            if ((packet.flags & FLAG_ACK) == FLAG_ACK && packet.ack == connection->curr_seq + 1) {
                LOG_INFO("Connection established");
                set_connection_established(1);
                connection->state = ESTABLISHED;
            } else {
                LOG_ERROR("Failed to establish connection! Expected ACK packet with ack_num: %u", connection->curr_seq + 1);
                connection->state = CLOSED;
            }
            break;
        case ESTABLISHED:
            // if packet seq_number not expected, resend previous ack.
            // else, send ACK for packet and process payload.
            if ((packet.flags & FLAG_FIN) == FLAG_FIN) {
                set_connection_established(0);

                connection->state = FIN_SENT;

                int result = send_ack_packet(socket_id, &client_addr, client_addr_len,
                    connection->curr_seq, packet.seq_num + 1);
                if (result != 0) {
                    return;
                }
                connection->curr_seq += 1;
                connection->next_expected += 1;
                connection->state = CLOSED;
            } else if (packet.flags & FLAG_DAT) {
                if (packet.seq_num == connection->next_expected) {
                    connection->next_expected += packet.payload_len;
                    int result = send_ack_packet(socket_id, &client_addr, client_addr_len,
                                connection->curr_seq, connection->next_expected);
                    if (result != 0) {
                        return;
                    }
                } else {
                    LOG_WARN("Received out of order packet. Expected seq_num: %u but got seq_num: %u",
                        connection->next_expected, packet.seq_num);
                }
            }
            break;
        default:
            // should not reach here
            break;
    }
}
