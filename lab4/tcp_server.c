#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include "sockets.c"

// NOTE: see man getaddrinfo
int main(int argc, char *argv[]) {
    char buffer[MAX_LENGTH];
    int socket_fd = socket(DOMAIN, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    struct sockaddr_in address; // IPv4 address
    address.sin_family = DOMAIN;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    printf("binding");
    bind(socket_fd, (struct sockaddr *) &address, sizeof(address));
    printf("listening");
    listen(socket_fd, 3); // max 3 requests in queue
    socklen_t length = sizeof(address);
    int n, client_socket;
    do {
        printf("accepting");
        client_socket = accept(socket_fd, (struct sockaddr *) &address, &length);
        printf("reading");
        n = read(client_socket, buffer, MAX_LENGTH);
        printf("read");
        while (!is_exit(buffer, n)) {
            if (strlen(buffer) > 0) {
                printf("Server got %s\n", buffer);
                process_message(buffer, n);
                printf("Server sent %s\n", buffer);
                write(client_socket, buffer, n);
            }
            n = read(client_socket, buffer, MAX_LENGTH);
        }
    } while (!is_exit(buffer, n));
    close(socket_fd);
}
/* explanation:
   i used to establish j. 1 client socket via 'accept' and listen to it
*/
