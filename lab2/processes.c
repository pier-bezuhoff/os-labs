#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/stat.h>

const char COUNTING_FIFO_FILENAME[] = "counting_fifo";
// NOTE: use semaphores in shm!

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

// 3 => no more than 999 processes expected
int get_count() {
    int counting_fifo = open(COUNTING_FIFO_FILENAME, O_RDONLY);
    char *line;
    read(counting_fifo, &line, 3);
    close(counting_fifo);
    int n;
    sscanf(line, "%d", &n);
    printf("count is %d", n);
    return n;
}

void set_count(int count) {
    int counting_fifo = open(COUNTING_FIFO_FILENAME, O_WRONLY);
    char n_str[3];
    sprintf(n_str, "%d", count);
    write(counting_fifo, n_str, strlen(n_str));
    close(counting_fifo);
    printf("count = %d", count);
}

void spawn_children(int process_id, int **graph) {
    printf("%d:\n", process_id);
    int child_id;
    for (int j = 0; (child_id = graph[process_id][j]) != -1; j++) {
        pid_t pid = fork();
        if (pid == 0) { // child
            printf("%d -> %d\n", process_id, child_id);
            set_count(get_count() + 1);
            spawn_children(child_id, graph);
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
    printf("Reading graph from %s", graph_filename);
    FILE *file = fopen(graph_filename, "r");
    if (file == NULL) {
        fprintf(stderr, "Unable to read graph file %s", graph_filename);
        exit(EXIT_FAILURE);
    }
    int n_processes = 0;
    while (!feof(file))
        if (fgetc(file) == '\n')
            n_processes++;
    int *graph[n_processes];
    for (int i = 0; i < n_processes; i++)
        graph[i] = (int *) malloc(((n_processes + 1) * sizeof(int)));
    read_graph(graph_filename, graph);
    if (mkfifo(COUNTING_FIFO_FILENAME, 0666) != 0) {
        fprintf(stderr, "Unable to create fifo %s", COUNTING_FIFO_FILENAME);
        exit(EXIT_FAILURE);
    }
    set_count(0);
    spawn_children(0, graph);
    printf("Started %d processes", get_count());
}
