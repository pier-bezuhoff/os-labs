#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <semaphore.h>
#include <signal.h>
#include <pthread.h>

int debug = 1;
int countdown = 0;
FILE *output; // default stdout
const int DEFAULT_COUNTDOWN = 5;
const int MIN_WIDTH = 2;
const int MAX_WIDTH = 6;
const int MIN_HEIGHT = 2;
const int MAX_HEIGHT = 4;
int width;
int height;
int *matrix1; // matrices are flattened
int *matrix2;
int *result_matrix;
sem_t on_generated, on_added, on_printed, on_done;
int end; // volatile?

int randrange(int min, int max) {
    return min + rand() / (RAND_MAX / (max - min));
}

void *start_generator(void *_) {
    time_t t;
    srand((unsigned) time(&t));
    sem_wait(&on_printed);
    while (!end) {
        width = randrange(MIN_WIDTH, MAX_WIDTH);
        height = randrange(MIN_HEIGHT, MAX_HEIGHT);
        for (int row = 0; row < height; row++) {
            for (int column = 0; column < width; column++) {
                int ix = row * width + column;
                matrix1[ix] = randrange(-100, 100);
                matrix2[ix] = randrange(-100, 100);
            }
        }
        sem_post(&on_generated);
        sem_wait(&on_printed);
    }
    return NULL;
}

void *start_adder(void *_) {
    sem_wait(&on_generated);
    while (!end) {
        for (int row = 0; row < height; row++) {
            for (int column = 0; column < width; column++) {
                int ix = row * width + column;
                result_matrix[ix] = matrix1[ix] + matrix2[ix];
            }
        }
        sem_post(&on_added);
        sem_wait(&on_generated);
    }
    return NULL;
}

void fprint_matrix(FILE *file, int *matrix) {
    for (int row = 0; row < height; row++) {
        for (int column = 0; column < width; column++) {
            fprintf(file, "%d ", matrix[row * width + column]);
        }
        fprintf(file, "\n");
    }
}

void *start_printer(void *_) {
    sem_wait(&on_added);
    while (!end) {
        fprint_matrix(output, matrix1);
        fprintf(output, " +\n");
        fprint_matrix(output, matrix2);
        fprintf(output, " =\n");
        fprint_matrix(output, result_matrix);
        fprintf(output, "\n");
        if (countdown)
            sem_post(&on_done);
        sem_post(&on_printed);
        sem_wait(&on_added);
    }
    fclose(output);
    return NULL;
}

void on_interruption(int signo) {
    end = 1; // notify all 3 processes
    if (debug)
        printf("Interrupted (%d)\n", signo);
    sem_post(&on_generated);
    sem_post(&on_added);
    sem_post(&on_printed);
    if (countdown)
        sem_post(&on_done);
    exit(EXIT_SUCCESS);
}

void *start_countdown(void *_) {
    int count = countdown;
    while (!end && count > 0) {
        sem_wait(&on_done);
        count--;
    }
    if (!end && debug)
        printf("Countdown forced interruption\n");
    on_interruption(SIGINT);
    return NULL;
}

void wrong_usage(char *cmd) {
    fprintf(stderr, "Usage: %s [-o outfile] [-v|-q] [-c countdown|-C]\n", cmd);
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
    output = stdout;
    int new_countdown;
    FILE *file;
    int opt;
    while ((opt = getopt(argc, argv, "o:vqc:C")) != -1) {
        switch (opt) {
        case 'o':
            file = fopen(optarg, "w");
            if (file != NULL)
                output = file;
            break;
        case 'v': debug = 1; break;
        case 'q': debug = 0; break;
        case 'c':
            if (optarg != NULL && sscanf(optarg, "%d", &new_countdown) == 1)
                countdown = new_countdown;
            else
                countdown = DEFAULT_COUNTDOWN;
            break;
        case 'C': countdown = 0; break;
        default: wrong_usage(argv[0]);
        }
    }
    if (optind < argc)
        wrong_usage(argv[0]);
    matrix1 = malloc(MAX_WIDTH * MAX_HEIGHT * sizeof(int));
    matrix2 = malloc(MAX_WIDTH * MAX_HEIGHT * sizeof(int));
    result_matrix = malloc(MAX_WIDTH * MAX_HEIGHT * sizeof(int));

    sem_init(&on_generated, 0, 0);
    sem_init(&on_added, 0, 0);
    sem_init(&on_printed, 0, 0);
    sem_init(&on_done, 0, 0);
    end = 0;
    // start
    signal(SIGINT, on_interruption);
    signal(SIGTERM, on_interruption);
    pthread_t generator, adder, printer, counter;
    pthread_create(&generator, NULL, start_generator, NULL);
    pthread_create(&adder, NULL, start_adder, NULL);
    pthread_create(&printer, NULL, start_printer, NULL);
    if (countdown)
        pthread_create(&counter, NULL, start_countdown, NULL);
    sem_post(&on_printed); // start generator
    // stop
    pthread_join(generator, NULL);
    pthread_join(adder, NULL);
    pthread_join(printer, NULL);
    if (countdown)
        pthread_join(counter, NULL);
}
