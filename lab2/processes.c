#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <semaphore.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <sys/stat.h>

const char *COUNTING_FIFO_FILENAME = "counting_fifo";
const int MAX_N_CHILDREN = 10;
const int MAX_N_DIGITS = 3; // max # of digits in process id
int **graph;
int n_lines; // # lines in graph.txt
// in shared memory:
sem_t *syncronization;
int *count; // # of processes
int *n_running;
int *potential;

/* Example:
1 3
2 3
0-th process starts 1-st and 3-rd; 1-st starts 2-nd and 3-rd
*/
void read_graph(char *graph_filename, int **graph) {
    FILE *file = fopen(graph_filename, "r");
    char *line = NULL;
    int i = 0; // line number
    size_t len = 0;
    ssize_t n_chars_read;
    if (file == NULL) {
        fprintf(stderr, "Unable to n_chars_read graph file %s", graph_filename);
        exit(EXIT_FAILURE);
    }
    while ((n_chars_read = getline(&line, &len, file)) != -1) {
        if (line[n_chars_read - 1] == '\n')
            line[n_chars_read - 1] = '\0';
        int j = 0;
        int process_id;
        for (char *token = strtok(line, " "); token != NULL; token = strtok(NULL, " ")) {
            sscanf(token, "%d", &process_id);
            graph[i][j] = process_id;
            j++;
        }
        for ( ; j < n_lines; j++)
            graph[i][j] = -1;
        graph[i][j] = -1; // indicate n_running
        i++;
    }
    fclose(file);
}

void spawn_children(int process_id);

void spawn_process(int process_id) {
    if (process_id < 0 || process_id > 10)
        fprintf(stderr, "strange process_id: %d\n", process_id);
    (*n_running)++;
    (*count)++;
    printf("(%d[%d]/%d) +%d\n", *n_running, *potential, *count, process_id);
    sem_post(syncronization);
    if (process_id < n_lines)
        spawn_children(process_id);
    sem_wait(syncronization);
    (*n_running)--;
    printf("(%d[%d]/%d) -%d\n", *n_running, *potential, *count, process_id);
    (*potential)--;
    if ((*potential) == 0)
        printf("%d processes spawned\n", *count);
    sem_post(syncronization);
    exit(EXIT_SUCCESS);
}

void spawn_children(int process_id) {
    int child_id;
    // -1 indicates n_running
    int j = 0;
    child_id = graph[process_id][j];
    while (child_id != -1) {
        (*potential)++;
        if (fork() == 0) { // child
            sem_wait(syncronization);
            printf("%d -> %d\n", process_id, child_id);
            spawn_process(child_id);
        }
        j++;
        child_id = graph[process_id][j];
    }
}

int main(int argc, char *argv[]) {
    char *graph_filename = "graph.txt";
    if (argc == 2) {
        graph_filename = argv[1];
    } else if (argc > 2) {
        fprintf(stderr, "Usage: %s [GRAPH]\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    // read graph length and graph
    FILE *file = fopen(graph_filename, "r");
    if (file == NULL) {
        fprintf(stderr, "Unable to read graph file %s\n", graph_filename);
        exit(EXIT_FAILURE);
    }
    n_lines = 0;
    while (!feof(file))
        if (fgetc(file) == '\n')
            n_lines++;
    graph = malloc(sizeof(int[n_lines]));
    for (int i = 0; i < n_lines; i++)
        graph[i] = malloc(sizeof(int[MAX_N_CHILDREN])); // each process starts <= MAX_N_CHILDREN children
    read_graph(graph_filename, graph);
    // setup shared memory
    syncronization = shmat(shmget(IPC_PRIVATE, sizeof(sem_t), IPC_CREAT | IPC_EXCL | 0666), NULL, 0);
    sem_init(syncronization, 1, 1);
    count = shmat(shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | IPC_EXCL | 0666), NULL, 0);
    *count = 0;
    potential = shmat(shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | IPC_EXCL | 0666), NULL, 0);
    *potential = 1; // strange, but works
    n_running = shmat(shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | IPC_EXCL | 0666), NULL, 0);
    *n_running = 0;
    // start
    spawn_process(0);
}
