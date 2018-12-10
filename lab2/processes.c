#include <unistd.h>
#include "graph.c"

const int DEBUG = 1;
const int MAX_N_CHILDREN = 10;
const int MAX_COUNT_LENGTH = 4; // no more than 9999
const char *DEFAULT_GRAPH_FILENAME = "test_graph1.txt";
int **graph;
int n_lines; // # of lines in graph file

int str2int(char *str) {
    int x;
    if (sscanf(str, "%d", &x) == 1)
        return x;
    else {
        fprintf(stderr, "not an int: %s", str);
        exit(EXIT_FAILURE);
    }
}

char *count2str(int x) {
    char *str = malloc(sizeof(char[MAX_COUNT_LENGTH]));
    sprintf(str, "%d", x);
    return str;
}

int spawn_children(int process_id);

void spawn_process(int process_id, int output) {
    if (DEBUG)
        printf("+%d\n", process_id);
    int count = 0;
    if (process_id < n_lines) // if >= then it has no children
        count = spawn_children(process_id);
    if (DEBUG)
        printf("-%d\n", process_id);
    write(output, count2str(count + 1), MAX_COUNT_LENGTH);
    exit(EXIT_SUCCESS);
}

int spawn_children(int process_id) {
    int child_id;
    // -1 indicates end
    int count = 0;
    int j = 0;
    child_id = graph[process_id][j];
    while (child_id != -1) {
        int io[2];
        char buffer[MAX_COUNT_LENGTH];
        pipe(io);
        if (fork() == 0) { // in child
            if (DEBUG)
                printf("%d -> %d\n", process_id, child_id);
            spawn_process(child_id, io[1]);
        }
        read(io[0], buffer, MAX_COUNT_LENGTH);
        count += str2int(buffer);
        j++;
        child_id = graph[process_id][j];
    }
    return count;
}

int main(int argc, char *argv[]) {
    const char *graph_filename = DEFAULT_GRAPH_FILENAME;
    if (argc == 2) {
        graph_filename = argv[1];
    } else if (argc > 2) {
        fprintf(stderr, "Usage: %s [graph_file]\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    // read graph
    graph = read_graph(graph_filename, MAX_N_CHILDREN);
    n_lines = count_lines(graph_filename);
    // start
    int io[2];
    pipe(io);
    if (fork() == 0)
        spawn_process(0, io[1]);
    char buffer[MAX_COUNT_LENGTH];
    read(io[0], buffer, MAX_COUNT_LENGTH);
    printf("%d processes spawned\n", str2int(buffer));
}
