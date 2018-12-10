#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>

int debug = 1;
int countdown = 5;
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
pthread_cond_t generated, added, printed, done;
pthread_mutex_t mutex;
int end; // volatile?

int randrange(int min, int max) {
    return min + rand() / (RAND_MAX / (max - min));
}

void *start_generator(void *_) {
    time_t t;
    srand((unsigned) time(&t));
    pthread_cond_wait(&printed, &mutex);
    while (!end) {
        if (debug)
            printf("generating...\n");
        width = randrange(MIN_WIDTH, MAX_WIDTH);
        height = randrange(MIN_HEIGHT, MAX_HEIGHT);
        for (int row = 0; row < height; row++) {
            for (int column = 0; column < width; column++) {
                int ix = row * width + column;
                matrix1[ix] = randrange(-100, 100);
                matrix2[ix] = randrange(-100, 100);
            }
        }
        pthread_cond_signal(&generated);
        pthread_cond_wait(&printed, &mutex);
    }
    pthread_mutex_unlock(&mutex);
    return NULL;
}

void *start_adder(void *_) {
    pthread_cond_wait(&generated, &mutex);
    while (!end) {
        if (debug)
            printf("adding...\n");
        for (int row = 0; row < height; row++) {
            for (int column = 0; column < width; column++) {
                int ix = row * width + column;
                result_matrix[ix] = matrix1[ix] + matrix2[ix];
            }
        }
        pthread_cond_signal(&added);
        pthread_cond_wait(&generated, &mutex);
    }
    pthread_mutex_unlock(&mutex);
    return NULL;
}

void print_matrix(int *matrix) {
    for (int row = 0; row < height; row++) {
        for (int column = 0; column < width; column++) {
            printf("%d ", matrix[row * width + column]);
        }
        printf("\n");
    }
}

void *start_printer(void *_) {
    pthread_cond_wait(&added, &mutex);
    while (!end) {
        if (debug)
            printf("printing...\n");
        print_matrix(matrix1);
        printf(" +\n");
        print_matrix(matrix2);
        printf(" =\n");
        print_matrix(result_matrix);
        printf("\n");
        if (countdown)
            pthread_cond_signal(&done);
        pthread_cond_signal(&printed);
        pthread_cond_wait(&added, &mutex);
    }
    pthread_mutex_unlock(&mutex);
    return NULL;
}

void on_interruption(int signo) {
    end = 1; // notify all 3 processes
    if (debug)
        printf("Interrupted (%d)\n", signo);
    pthread_cond_broadcast(&generated);
    pthread_cond_broadcast(&added);
    pthread_cond_broadcast(&printed);
    if (countdown)
        pthread_cond_broadcast(&done);
    exit(EXIT_SUCCESS);
}

void *start_countdown(void *_) {
    int countdown = 5;
    while (!end && countdown > 0) {
        pthread_cond_wait(&done, &mutex);
        countdown--;
    }
    if (!end && debug)
        printf("Countdown forced interruption\n");
    on_interruption(SIGINT);
    pthread_mutex_unlock(&mutex);
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
    end = 0;
    pthread_cond_init(&generated, NULL);
    pthread_cond_init(&added, NULL);
    pthread_cond_init(&printed, NULL);
    pthread_cond_init(&done, NULL);
    // start
    signal(SIGINT, on_interruption);
    signal(SIGTERM, on_interruption);
    pthread_t counter, generator, adder, printer;
    pthread_create(&generator, NULL, start_generator, NULL);
    pthread_create(&adder, NULL, start_adder, NULL);
    pthread_create(&printer, NULL, start_printer, NULL);
    pthread_cond_signal(&printed); // start generator
    if (countdown)
        pthread_create(&counter, NULL, start_countdown, NULL);
    pthread_join(generator, NULL);
    pthread_join(adder, NULL);
    pthread_join(printer, NULL);
    if (countdown)
        pthread_join(counter, NULL);
}
