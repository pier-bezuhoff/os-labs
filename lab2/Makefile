CC = gcc
CFLAGS = -Wall -pthread
OBJECTS = processes.o threads.o

$(OBJECTS): graph.c

processes: processes.o
	$(CC) $(CFLAGS) -o spawn_processes processes.o

threads: threads.o
	$(CC) $(CFLAGS) -o spawn_threads threads.o

.PHONY: all
all: processes threads
.DEFAULT_GOAL := all

.PHONY: clean
clean:
	-rm $(OBJECTS) spawn_processes spawn_threads
