#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include "sockets.c"

int main(int argc, char *argv[]) {
    char buffer[MAX_LENGTH];
    int socket_fd = socket(DOMAIN, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));
    struct sockaddr_in address;
    address.sin_family = DOMAIN;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    bind(socket_fd, (struct sockaddr *) &address, sizeof(address));
    listen(socket_fd, 3);
    socklen_t length = sizeof(address);
    int client_socket;
    int n;
    while (1) {
        client_socket = accept(socket_fd, (struct sockaddr *) &address, &length);
        n = read(client_socket, buffer, MAX_LENGTH);
        printf("Server got %s\n", buffer);
        if (is_exit(buffer, n))
            break;
        process_message(buffer, n);
        printf("Server sent %s\n", buffer);
        write(client_socket, buffer, n);
    };
    close(socket_fd);
}
