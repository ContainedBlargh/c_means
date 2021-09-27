CC=gcc
CFLAGS=-std=gnu99 -ggdb -Wall -pedantic -O3

main: main.o
	$(CC) -o c_means fail.c util.c k_means.c main.o -lm
	
