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
// in shared memory:
int ***graph;
sem_t *on_child; // on when child is waiting for input
sem_t *on_got_current_count; // on fifo returning current count to child
sem_t *on_sent_new_count; // on child sending new count to fifo
int *n_running; // 3 of running processes

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
        graph[i][j] = -1; // indicate end
        i++;
    }
    fclose(file);
}

int fifo_fetch_count() {
    int counting_fifo = open(COUNTING_FIFO_FILENAME, O_RDONLY);
    char *line;
    read(counting_fifo, &line, MAX_N_DIGITS);
    close(counting_fifo);
    int n;
    sscanf(line, "%d", &n);
    printf("fetched count: %d\n", n);
    return n;
}

void fifo_send_count(int count) {
    int counting_fifo = open(COUNTING_FIFO_FILENAME, O_WRONLY);
    char n_str[MAX_N_DIGITS];
    sprintf(n_str, "%d", count);
    write(counting_fifo, n_str, strlen(n_str));
    close(counting_fifo);
    printf("sended count: %d\n", count);
}

void spawn_counter() {
    if (fork() == 0) { // in child
        if (mkfifo(COUNTING_FIFO_FILENAME, 0666) != 0) {
            fprintf(stderr, "Unable to create fifo %s\n", COUNTING_FIFO_FILENAME);
            exit(EXIT_FAILURE);
        }
        int n = 0; // internal counter
        sem_wait(on_child);
        while ((*n_running) > 0) {
            printf("counter sending %d...", n);
            fifo_send_count(n);
            sem_post(on_got_current_count);
            sem_wait(on_sent_new_count);
            n = fifo_fetch_count();
            printf("new count: %d", n);
            sem_wait(on_child);
        }
        printf("%d processes spawned\n", n);
    }
}

void spawn_children(int process_id);

void spawn_process(int process_id) {
    printf("%d:\n", process_id);
    sem_post(on_child);
    sem_post(on_child);
    sem_wait(on_got_current_count);
    printf("fetching current count...");
    int n = fifo_fetch_count();
    fifo_send_count(n + 1);
    printf("sending new count %d", n + 1);
    sem_post(on_sent_new_count);
    spawn_children(process_id);
    (*n_running)--;
}

void spawn_children(int process_id) {
    int child_id;
    // -1 indicates end
    for (int j = 0; (child_id = (*graph)[process_id][j]) != -1; j++) {
        (*n_running)++;
        if (fork() == 0) { // child
            printf("%d -> %d\n", process_id, child_id);
            spawn_process(child_id);
            break;
        }
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
    int n_lines = 0;
    while (!feof(file))
        if (fgetc(file) == '\n')
            n_lines++;
    int *local_graph[n_lines];
    for (int i = 0; i < n_lines; i++)
        local_graph[i] = malloc(sizeof(int[MAX_N_CHILDREN])); // each process starts <= MAX_N_CHILDREN children
    read_graph(graph_filename, local_graph);
    // setup shared memory
    graph = shmat(shmget(IPC_PRIVATE, sizeof(int[n_lines][MAX_N_CHILDREN]), IPC_CREAT | IPC_EXCL | 0666), NULL, 0);
    *graph = local_graph;
    on_child = shmat(shmget(IPC_PRIVATE, sizeof(sem_t), IPC_CREAT | IPC_EXCL | 0666), NULL, 0);
    sem_init(on_child, 1, 0);
    on_got_current_count = shmat(shmget(IPC_PRIVATE, sizeof(sem_t), IPC_CREAT | IPC_EXCL | 0666), NULL, 0);
    sem_init(on_got_current_count, 1, 0);
    on_sent_new_count = shmat(shmget(IPC_PRIVATE, sizeof(sem_t), IPC_CREAT | IPC_EXCL | 0666), NULL, 0);
    sem_init(on_sent_new_count, 1, 0);
    n_running = shmat(shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | IPC_EXCL | 0666), NULL, 0);
    *n_running = 0;
    // start
    spawn_counter();
    (*n_running)++;
    spawn_process(0);
}
