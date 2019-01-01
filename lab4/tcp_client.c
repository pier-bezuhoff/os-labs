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
<<<<<<< HEAD
    while (1) {
        char *message = NULL;
        size_t length = 0;
        getline(&message, &length, stdin);
        char buffer[MAX_LENGTH];
        int socket_fd = socket(DOMAIN, SOCK_STREAM, 0);
        struct sockaddr_in address;
        memset(&address, '0', sizeof(address));
=======
    char buffer[MAX_LENGTH];
    int socket_fd;
    char *message;
    size_t length;
    ssize_t n;
    do {
        // setup socket
        socket_fd = socket(DOMAIN, SOCK_STREAM, 0);
        struct sockaddr_in address;
        /* memset(&address, '0', sizeof(address)); */
>>>>>>> 91df727f78d4297751a5a8b876ba0b3a3ee274f9
        address.sin_family = DOMAIN;
        address.sin_port = htons(PORT);
        inet_pton(DOMAIN, ip_address, &address.sin_addr);
        if (connect(socket_fd, (struct sockaddr *) &address, sizeof(address)) < 0) {
            perror("TCP client: connection failed");
            exit(EXIT_FAILURE);
        }
<<<<<<< HEAD
        printf("Client is sending %s\n", message);
        if (is_exit(message, length))
            break;
        write(socket_fd, message, length);
        read(socket_fd, buffer, MAX_LENGTH);
        printf("Client got %s\n", buffer);
        close(socket_fd);
    }
=======
        // send, receive message
        message = NULL;
        length = 0;
        n = getline(&message, &length, stdin);
        printf("Client is sending %s\n", message);
        bzero(buffer, MAX_LENGTH);
        read(socket_fd, buffer, MAX_LENGTH);
        printf("Client got %s\n", buffer);
        close(socket_fd);
    } while (!is_exit(message, n));
>>>>>>> 91df727f78d4297751a5a8b876ba0b3a3ee274f9
}
