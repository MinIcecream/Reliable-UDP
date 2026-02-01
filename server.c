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

    const uint32_t WINDOW_SIZE = 1024;
    char buffer[WINDOW_SIZE];
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    while (1) {
        ssize_t received_msg_len = recvfrom(socket_id, buffer, sizeof(buffer), 0, (struct sockaddr *)&client_addr, &client_addr_len);
        if (received_msg_len < 0) {
            perror("Receive failed");
            continue;
        }
        printf("Received message: %.*s \n", (int)received_msg_len, buffer);
        int sent_msg_len = sendto(socket_id, buffer, received_msg_len, 0, (const struct sockaddr *)&client_addr, client_addr_len);
        printf("Echoed %d bytes back to client\n", sent_msg_len);
    }
    return 0;
}