#include "transport.h"

#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "common.h"
#include "packet.h"
#include "logger.h"

static int connection_established = 0;

void set_connection_established(int established) {
    connection_established = established;
}

static int should_drop_packet(uint32_t flags) {
    // Never drop packets during handshake
    if (!connection_established) {
        return 0;
    }
    
    int drop_percent = 0;
    if (flags & FLAG_ACK) {
        drop_percent = DROP_ACK_PERCENT;
    } else if (flags & FLAG_DAT) {
        drop_percent = DROP_DAT_PERCENT;
    } else if (flags & FLAG_FIN) {
        drop_percent = DROP_FIN_PERCENT;
    }
    
    if (drop_percent == 0) return 0;
    
    int random_value = rand() % 100;
    return random_value < drop_percent;
}

int send_packet(tcp_packet_t packet, int socket_id, struct sockaddr_in *destination_addr, socklen_t destination_addr_len) {
    LOG_INFO("Sending: %s", to_string(packet));

    if (should_drop_packet(packet.flags)) {
        LOG_WARN("[DROP] Dropped %s", to_string(packet));
        return 0;
    }
    
    packet.flags = htonl(packet.flags);
    packet.seq_num = htonl(packet.seq_num);
    packet.ack = htonl(packet.ack);
    packet.payload_len = htons(packet.payload_len);
    ssize_t sent_bytes = sendto(socket_id, &packet, sizeof(packet), 0, (const struct sockaddr *)destination_addr, destination_addr_len);
    if (sent_bytes < 0) {
        LOG_SYS_ERROR("Failed to send packet: %s", to_string(packet));
        return 1;
    }
    return 0;
}
