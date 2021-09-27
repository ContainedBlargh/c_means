CC=gcc
CFLAGS=-std=gnu99 -ggdb -Wall -pedantic

main: main.o
	$(CC) -o k_means fail.c util.c k_means.c main.o -lm
	
