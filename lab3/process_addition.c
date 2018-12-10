#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <semaphore.h>
#include <signal.h>
#include <sys/shm.h>

int debug = 1;
int countdown = 0;
FILE *output; // default stdout
const int DEFAULT_COUNTDOWN = 5;
const int MIN_WIDTH = 2;
const int MAX_WIDTH = 6;
const int MIN_HEIGHT = 2;
const int MAX_HEIGHT = 4;
// in shared memory:
int *width;
int *height;
int *matrix1; // matrices are flattened
int *matrix2;
int *result_matrix;
sem_t *on_generated;
sem_t *on_added;
sem_t *on_printed;
sem_t *on_done;
int *end; // volatile?

int randrange(int min, int max) {
    return min + rand() / (RAND_MAX / (max - min));
}

void spawn_generator() {
    if (fork() == 0) {
        time_t t;
        srand((unsigned) time(&t));
        sem_wait(on_printed);
        while (!*end) {
            *width = randrange(MIN_WIDTH, MAX_WIDTH);
            *height = randrange(MIN_HEIGHT, MAX_HEIGHT);
            for (int row = 0; row < *height; row++) {
                for (int column = 0; column < *width; column++) {
                    int ix = row * *width + column;
                    matrix1[ix] = randrange(-100, 100);
                    matrix2[ix] = randrange(-100, 100);
                }
            }
            sem_post(on_generated);
            sem_wait(on_printed);
        }
        exit(EXIT_SUCCESS);
    }
}

void spawn_adder() {
    if (fork() == 0) {
        sem_wait(on_generated);
        while (!*end) {
            for (int row = 0; row < *height; row++) {
                for (int column = 0; column < *width; column++) {
                    int ix = row * *width + column;
                    result_matrix[ix] = matrix1[ix] + matrix2[ix];
                }
            }
            sem_post(on_added);
            sem_wait(on_generated);
        }
        exit(EXIT_SUCCESS);
    }
}

void fprint_matrix(FILE *file, int *matrix) {
    for (int row = 0; row < *height; row++) {
        for (int column = 0; column < *width; column++) {
            fprintf(file, "%d ", matrix[row * *width + column]);
        }
        fprintf(file, "\n");
    }
}

void spawn_printer() {
    if (fork() == 0) {
        sem_wait(on_added);
        while (!*end) {
            fprint_matrix(output, matrix1);
            fprintf(output, " +\n");
            fprint_matrix(output, matrix2);
            fprintf(output, " =\n");
            fprint_matrix(output, result_matrix);
            fprintf(output, "\n");
            sem_post(on_done);
            sem_post(on_printed);
            sem_wait(on_added);
        }
        exit(EXIT_SUCCESS);
    }
}

void *shm_alloc(size_t size) {
    return shmat(shmget(IPC_PRIVATE, size, IPC_CREAT | IPC_EXCL | 0666), NULL, 0);
}

void sem_create(sem_t **semaphore, int initial_value) {
    *semaphore = shm_alloc(sizeof(sem_t));
    sem_init(*semaphore, 1, initial_value);
}

void on_interruption(int signo) {
    *end = 1; // notify all 3 processes
    sem_post(on_printed);
    sem_post(on_generated);
    sem_post(on_added);
    if (debug)
        printf("Interrupted (%d)\n", signo);
    exit(EXIT_SUCCESS);
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
    // setup shared memory
    width = shm_alloc(sizeof(int));
    height = shm_alloc(sizeof(int));
    matrix1 = shm_alloc(MAX_WIDTH * MAX_HEIGHT * sizeof(int));
    matrix2 = shm_alloc(MAX_WIDTH * MAX_HEIGHT * sizeof(int));
    result_matrix = shm_alloc(MAX_WIDTH * MAX_HEIGHT * sizeof(int));
    sem_create(&on_generated, 0);
    sem_create(&on_added, 0);
    sem_create(&on_printed, 0);
    sem_create(&on_done, 0);
    end = shm_alloc(sizeof(int));
    *end = 0;
    // start
    signal(SIGINT, on_interruption);
    signal(SIGTERM, on_interruption);
    spawn_generator();
    spawn_adder();
    spawn_printer();
    sem_post(on_printed); // invoke generator
    // stop
    int count = countdown;
    while (!countdown || count > 0) {
        sem_wait(on_done);
        count--;
    }
    if (debug)
        printf("Countdown forced interruption\n");
    on_interruption(SIGINT);
}
