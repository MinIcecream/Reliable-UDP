#ifndef TRANSPORT_H
#define TRANSPORT_H

#include <netinet/in.h>
#include <sys/socket.h>

#include "packet.h"

int send_packet(tcp_packet_t packet, int socket_id, struct sockaddr_in *destination_addr, socklen_t destination_addr_len);
void set_connection_established(int established);

#endif // TRANSPORT_H
