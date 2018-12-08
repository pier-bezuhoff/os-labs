#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <semaphore.h>
#include <signal.h>
#include <sys/shm.h>

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

void print_matrix(int *matrix) {
    for (int row = 0; row < *height; row++) {
        for (int column = 0; column < *width; column++) {
            printf("%d ", matrix[row * *width + column]);
        }
        printf("\n");
    }
}

void spawn_printer() {
    if (fork() == 0) {
        sem_wait(on_added);
        while (!*end) {
            print_matrix(matrix1);
            printf(" +\n");
            print_matrix(matrix2);
            printf(" =\n");
            print_matrix(result_matrix);
            printf("\n");
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
    exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[]) {
    // argc == 2 => read matrix from file
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
    spawn_generator();
    spawn_adder();
    spawn_printer();
    sem_post(on_printed); // invoke generator
    // stop
    int countdown = 5;
    while (1 /*countdown > 0*/) {
        sem_wait(on_done);
        countdown--;
    }
    on_interruption(0);
}
