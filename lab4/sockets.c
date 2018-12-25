#include <string.h>
#include <ctype.h>
#include <netinet/in.h>

const int DOMAIN = AF_INET; // IPv4
const int PORT = 8080;
const int MAX_LENGTH = 100; // max length of message between server and client

char *process_message(char *message, int length) {
    for (int i = 0; i < length; i++)
        message[i] = toupper(message[i]);
    return message;
}

int is_exit(char *str, size_t length) {
    return (strncmp(str, "exit", length) == 0) || (strncmp(str, "quit", length) == 0) || (strncmp(str, "exit\n", length) == 0) || (strncmp(str, "quit\n", length) == 0);
}
