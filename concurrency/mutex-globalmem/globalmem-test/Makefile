# Makefile by HLY

all: myapp

# Which compiler
CC = gcc

# Where are include files
INCLUDE = .

# Options for development
CFLAGS = -g -Wall -ansi

myapp: app.o
	$(CC) -o myapp app.o

clean:
	rm -rf *.o myapp

