#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include "sockets.c"

int main(int argc, char *argv[]) {
    char *ip_address = "127.0.0.1";
    if (argc > 1)
        ip_address = argv[1];
    while (1) {
        char *message = NULL;
        size_t length = 0;
        getline(&message, &length, stdin);
        char buffer[MAX_LENGTH];
        int socket_fd = socket(DOMAIN, SOCK_STREAM, 0);
        struct sockaddr_in address;
        memset(&address, '0', sizeof(address));
        address.sin_family = DOMAIN;
        address.sin_port = htons(PORT);
        inet_pton(DOMAIN, ip_address, &address.sin_addr);
        if (connect(socket_fd, (struct sockaddr *) &address, sizeof(address)) < 0) {
            perror("TCP client: connection failed");
            exit(EXIT_FAILURE);
        }
        printf("Client is sending %s\n", message);
        if (is_exit(message, length))
            break;
        write(socket_fd, message, length);
        read(socket_fd, buffer, MAX_LENGTH);
        printf("Client got %s\n", buffer);
        close(socket_fd);
    }
}
