#ifndef COMMON_H
#define COMMON_H

#define PORT 9001
#define TIMEOUT_SECONDS 3

#define LOG_PATH "reliable_udp.log"

// Packet drop simulation (percentage: 0-100)
#define DROP_ACK_PERCENT 0
#define DROP_DAT_PERCENT 50
#define DROP_FIN_PERCENT 0

#endif // COMMON_H