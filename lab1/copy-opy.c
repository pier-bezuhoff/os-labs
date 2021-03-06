#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

const int BUFFSIZE = 512; // in bytes
const int MAX_PATH_LENGTH = 128; // in chars

void wrong_usage(char *cmd) {
    fprintf(stderr, "Usage: %s [-f] [-i] source [source [...]] destination\n", cmd);
    exit(EXIT_FAILURE);
}

int copy_file_to_file(int input, char *destination) {
    int output = open(destination, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
    if (output < 0) {
        fprintf(stderr, "No access to destination %s, skipping...\n", destination);
        return EXIT_FAILURE;
    }
    char buffer[BUFFSIZE];
    int bytes;
    while ((bytes = read(input, buffer, BUFFSIZE)) > 0) {
        write(output, buffer, bytes);
    }
    if (close(input) < 0) {
        fprintf(stderr, "Input closed unsuccessfully\n");
    }
    if (close(output) < 0) {
        fprintf(stderr, "Output closed unsuccessfully\n");
    }
    return EXIT_SUCCESS;
}

int copy(char source[], char destination[], int force, int interactive) {
    printf("Copying %s to %s...\n", source, destination);
    int input = open(source, O_RDONLY);
    if (input < 0) {
        fprintf(stderr, "No access to source %s, skipping...\n", source);
        return EXIT_FAILURE;
    } else {
        if (source == destination) {
            printf("source == destination == %s, nothing written\n", source);
            return EXIT_SUCCESS;
        }
        struct stat dest;
        int exists = stat(destination, &dest) == 0;
        if (exists) {
            char file_destination[MAX_PATH_LENGTH];
            strcpy(file_destination, destination);
            // dir => must exist!
            if (S_ISDIR(dest.st_mode)) {
                if (destination[strlen(destination) - 1] != '/') {
                    strcat(file_destination, "/");
                }
                strcat(file_destination, source);
            }
            if (stat(file_destination, &dest) == 0) {
                if (interactive) {
                    char answer[4]; // for "yes\n"
                    printf("Destination %s already exists, overwrite? (Y/n) ", file_destination);
                    fgets(answer, 4, stdin);
                    if (tolower(answer[0]) == 'n') {
                        printf("Nothing written\n");
                        return EXIT_SUCCESS;
                    } else {
                        return copy_file_to_file(input, file_destination);
                    }
                } else if (!force) {
                    printf("Destination %s already exists, nothing written\n", file_destination);
                    return EXIT_SUCCESS;
                } else {
                    return copy_file_to_file(input, file_destination);
                }
            } else {
                return copy_file_to_file(input, file_destination);
            }
        } else {
            return copy_file_to_file(input, destination);
        }
    }
}

int main(int argc, char *argv[]) {
    int force = 0, interactive = 0;
    int opt;
    while ((opt = getopt(argc, argv, "fi")) != -1) {
        switch (opt) {
        case 'f': force = 1; break;
        case 'i': interactive = 1; break;
        default: wrong_usage(argv[0]);
        }
    }
    if (argc - optind < 2) {
        fprintf(stderr, "Expected source... destination after options\n");
        wrong_usage(argv[0]);
    }
    int n_sources = argc - optind - 1;
    char *sources[n_sources];
    int i;
    for (i = optind; i < argc - 1; i++) {
        sources[i - optind] = argv[i];
    }
    char *destination = argv[argc - 1];
    struct stat dest;
    if (n_sources > 1 && stat(destination, &dest) == 0 && !S_ISDIR(dest.st_mode)) {
        fprintf(stderr, "When multiple sources, destination %s should be a directory!\nNothing written\n", destination);
        exit(EXIT_FAILURE);
    }
    if (n_sources > 1 && access(destination, F_OK) == -1) {
        if (mkdir(destination, S_IRWXU | S_IRWXG) < 0) {
            fprintf(stderr, "Failed to create destination directory %s\n", destination);
            exit(EXIT_FAILURE);
        }
    }
    for (i = 0; i < n_sources; i++) {
        copy(sources[i], destination, force, interactive);
    }
    printf("Done\n");
    return EXIT_SUCCESS;
}
