#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Example:
1 3
2 3
0-th process starts 1-st and 3-rd; 1-st starts 2-nd and 3-rd; 2-nd and 3-rd do nothing
*/
int **read_graph(const char *graph_filename, int width) {
    FILE *file = fopen(graph_filename, "r");
    if (file == NULL) {
        fprintf(stderr, "Unable to read graph file %s", graph_filename);
        exit(EXIT_FAILURE);
    }
    // count lines to allocate graph
    int n_lines = 0;
    while (!feof(file))
        if (fgetc(file) == '\n')
            n_lines++;
    int **graph = malloc(n_lines * sizeof(int *));
    for (int i = 0; i < n_lines; i++)
        graph[i] = malloc(sizeof(int[width]));
    // read graph
    rewind(file);
    char *line = NULL;
    int i = 0; // line number
    size_t len = 0;
    ssize_t n_chars_read;
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
        for ( ; j <= n_lines; j++)
            graph[i][j] = -1; // indicate end
        i++;
    }
    fclose(file);
    return graph;
}

int count_lines(const char *graph_filename) {
    FILE *file = fopen(graph_filename, "r");
    if (file == NULL) {
        fprintf(stderr, "Unable to read graph file %s", graph_filename);
        exit(EXIT_FAILURE);
    }
    int n_lines = 0;
    while (!feof(file))
        if (fgetc(file) == '\n')
            n_lines++;
    return n_lines;
}
