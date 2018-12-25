#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include "sockets.c"

int main(int argc, char *argv[]) {
    char *message = NULL;
    size_t length = 0;
    char buffer[MAX_LENGTH];
    int socket_fd = socket(DOMAIN, SOCK_DGRAM, 0);
    struct sockaddr_in server_address;
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = DOMAIN;
    server_address.sin_port = htons(PORT);
    server_address.sin_addr.s_addr = INADDR_ANY;
    ssize_t n = getline(&message, &length, stdin);
    socklen_t address_length = sizeof(server_address);
    sendto(socket_fd, (const char *) message, length, MSG_CONFIRM, (const struct sockaddr *) &server_address, address_length);
    printf("Client sent %s\n", message);
    while (!is_exit(message, length)) {
        recvfrom(socket_fd, (char *) buffer, MAX_LENGTH, MSG_WAITALL, (struct sockaddr *) &server_address, &address_length);
        printf("Client got %s\n", buffer);
        n = getline(&message, &length, stdin);
        sendto(socket_fd, (const char *) message, length, MSG_CONFIRM, (const struct sockaddr *) &server_address, address_length);
        printf("Client sent %s\n", message);
    }
    close(socket_fd);
}
