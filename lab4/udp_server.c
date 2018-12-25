#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>
#include "sockets.c"

int main(int argc, char *argv[]) {
    struct sockaddr_in address, client_address;
    memset(&address, 0, sizeof(address));
    memset(&client_address, 0, sizeof(client_address));
    char buffer[MAX_LENGTH];
    int socket_fd = socket(DOMAIN, SOCK_DGRAM, 0);
    if (socket_fd < 0) {
        perror("socket() failed");
        exit(EXIT_FAILURE);
    }
    address.sin_family = DOMAIN;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    if (bind(socket_fd, (const struct sockaddr *) &address, sizeof(address)) < 0) {
        perror("bind() failed");
        exit(EXIT_FAILURE);
    }
    socklen_t length = sizeof(client_address);
    size_t n = recvfrom(socket_fd, (char *) buffer, MAX_LENGTH, MSG_WAITALL, (struct sockaddr *) &client_address, &length);
    printf("Server got %s\n", buffer);
    while (!is_exit(buffer, n)) {
        process_message(buffer, n);
        sendto(socket_fd, (const char *) buffer, sizeof(buffer), MSG_CONFIRM, (const struct sockaddr *) &client_address, sizeof(client_address));
        printf("Server sent %s\n", buffer);
        n = recvfrom(socket_fd, (char *) buffer, MAX_LENGTH, MSG_WAITALL, (struct sockaddr *) &client_address, &length);
        buffer[n] = '\0';
        printf("Server got %s\n", buffer);
    }
    close(socket_fd);
}
