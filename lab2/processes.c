#include <unistd.h>
#include <semaphore.h>
#include <sys/shm.h>
#include "graph.c"

const int DEBUG = 1;
const int MAX_N_CHILDREN = 10;
const char *DEFAULT_GRAPH_FILENAME = "test_graph1.txt";
int **graph;
int n_lines; // # of lines in graph file
// in shared memory:
sem_t *syncronization;
sem_t *on_end;
int *count; // # of processes
int *n_running;
int *potential;

void spawn_children(int process_id);

void spawn_process(int process_id) {
    (*count)++;
    (*n_running)++;
    if (DEBUG)
        printf("(%d[%d]/%d) +%d\n", *n_running, *potential, *count, process_id);
    sem_post(syncronization);
    if (process_id < n_lines) // if >= then it has no children
        spawn_children(process_id);
    sem_wait(syncronization);
    (*n_running)--;
    if (DEBUG)
        printf("(%d[%d]/%d) -%d\n", *n_running, *potential, *count, process_id);
    (*potential)--;
    if ((*potential) == 0)
        sem_post(on_end);
    sem_post(syncronization);
    exit(EXIT_SUCCESS);
}

void spawn_children(int process_id) {
    int child_id;
    // -1 indicates end
    int j = 0;
    child_id = graph[process_id][j];
    while (child_id != -1) {
        (*potential)++;
        if (fork() == 0) { // in child
            sem_wait(syncronization);
            if (DEBUG)
                printf("%d -> %d\n", process_id, child_id);
            spawn_process(child_id);
        }
        j++;
        child_id = graph[process_id][j];
    }
}

void *shm_alloc(int size) {
    return shmat(shmget(IPC_PRIVATE, size, IPC_CREAT | IPC_EXCL | 0666), NULL, 0);
}

int main(int argc, char *argv[]) {
    const char *graph_filename = DEFAULT_GRAPH_FILENAME;
    if (argc == 2) {
        graph_filename = argv[1];
    } else if (argc > 2) {
        fprintf(stderr, "Usage: %s [GRAPH]\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    // read graph
    graph = read_graph(graph_filename, MAX_N_CHILDREN);
    n_lines = count_lines(graph_filename);
    // setup shared memory
    syncronization = shm_alloc(sizeof(sem_t));
    sem_init(syncronization, 1, 0);
    on_end = shm_alloc(sizeof(sem_t));
    sem_init(on_end, 1, 0);
    count = shm_alloc(sizeof(int));
    *count = 0;
    potential = shm_alloc(sizeof(int));
    *potential = 1; // inc in spawn_children
    n_running = shm_alloc(sizeof(int));
    *n_running = 0; // inc in spawn_process
    // start
    if (fork() == 0)
        spawn_process(0);
    sem_wait(on_end);
    printf("%d processes spawned\n", *count);
}
