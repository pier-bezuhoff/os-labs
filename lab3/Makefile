CC = gcc
CFLAGS = -Wall -pthread
OBJECTS = process_addition.o

process_addition: process_addition.o
	$(CC) $(CFLAGS) -o process_addition process_addition.o

.PHONY: all
all: process_addition
.DEFAULT_GOAL := all

.PHONY: clean
clean:
	-rm $(OBJECTS) process_addition
