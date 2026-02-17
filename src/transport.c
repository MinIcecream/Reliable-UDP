#include "transport.h"

#include <arpa/inet.h>
#include <stdio.h>

int send_packet(tcp_packet_t packet, int socket_id, struct sockaddr_in *destination_addr, socklen_t destination_addr_len) {
    packet.flags = htonl(packet.flags);
    packet.seq_num = htonl(packet.seq_num);
    packet.ack = htonl(packet.ack);
    packet.payload_len = htons(packet.payload_len);

    ssize_t sent_bytes = sendto(socket_id, &packet, sizeof(packet), 0, (const struct sockaddr *)destination_addr, destination_addr_len);
    if (sent_bytes < 0) {
        perror("Failed to send SYN packet");
        return 1;
    }
    return 0;
}
