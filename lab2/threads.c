#include <pthread.h>
#include <stdint.h>
#include "graph.c"

const int DEBUG = 1;
const int MAX_N_CHILDREN = 10;
const char *DEFAULT_GRAPH_FILENAME = "test_graph1.txt";
int **graph;
int n_lines; // # of lines in graph.txt
int count; // # of threads
int n_running;

void spawn_children(int thread_id);

void *start_thread(void *thread_id_ptr) {
    int thread_id = (intptr_t) thread_id_ptr;
    count++;
    n_running++;
    if (DEBUG)
        printf("(%d/%d) +%d\n", n_running, count, thread_id);
    if (thread_id < n_lines)
        spawn_children(thread_id);
    n_running--;
    if (DEBUG)
        printf("(%d/%d) -%d\n", n_running, count, thread_id);
    return NULL;
}

void spawn_children(int thread_id) {
    int child_id;
    int j = 0;
    child_id = graph[thread_id][j];
    // -1 indicates end
    while (child_id != -1) {
        pthread_t child;
        if (DEBUG)
            printf("%d -> %d\n", thread_id, child_id);
        pthread_create(&child, NULL, start_thread, (void *) (intptr_t) child_id);
        pthread_join(child, NULL);
        j++;
        child_id = graph[thread_id][j];
    }
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
    count = 0;
    n_running = 0;
    // start
    start_thread(0);
    printf("%d threads spawned\n", count);
}
