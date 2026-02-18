#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include "common.h"
#include "connection.h"
#include <stdlib.h>
#include <sys/select.h>
#include "transport.h"
#include "logger.h"

int main() {
    log_init(LOG_SOURCE_CLIENT, LOG_PATH);
    int socket_id = socket(AF_INET, SOCK_DGRAM, 0);

    if (socket_id < 0) {
        LOG_SYS_ERROR("Socket creation failed: %s", strerror(errno));
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
    connection.send_base = connection.initial_seq - 1;

    // Initiate connection
    tcp_packet_t syn_packet;
    memset(&syn_packet, 0, sizeof(syn_packet));
    syn_packet.flags = FLAG_SYN;
    syn_packet.seq_num = connection.curr_seq;
    int result = send_packet(syn_packet, socket_id, &server_addr, server_addr_len);
    if (result != 0) {
        LOG_ERROR("Failed to send SYN packet");
        return 1;
    }
    LOG_INFO("Sent SYN packet with seq_num: %u", connection.curr_seq);
    connection.curr_seq += 1;
    connection.send_base += 1;
    connection.state = SYN_SENT;

    char buffer[MAX_PACKET_SIZE];
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
                LOG_SYS_ERROR("Receive failed: %s", strerror(errno));
                continue;
            }
            tcp_packet_t packet;
            deserialize_packet(buffer, &packet);
            client_handle_state(&connection, packet, socket_id, server_addr, server_addr_len);
        }
        else if (ret == 0) {
            client_handle_timeout(&connection, socket_id, server_addr, server_addr_len);
        }
        else {
            LOG_SYS_ERROR("Select failed: %s", strerror(errno));
        }
      }
    return 0;    
}