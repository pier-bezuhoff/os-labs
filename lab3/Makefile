CC = gcc
CFLAGS = -Wall -pthread
OBJECTS = process_addition.o thread_addition.o
EXECUTABLES = process_addition thread_addition

process_addition: process_addition.o
	$(CC) $(CFLAGS) -o $@ $^

thread_addition: thread_addition.o
	$(CC) $(CFLAGS) -o $@ $^

.PHONY: all
all: $(EXECUTABLES)
.DEFAULT_GOAL := all

.PHONY: clean
clean:
	-rm $(OBJECTS) $(EXECUTABLES)
