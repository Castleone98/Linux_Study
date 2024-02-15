CC=gcc
CFLAGS=-Wall

all: numgen fork thread

numgen: numgen.c
	$(CC) $(CFLAGS) -o numgen numgen.c

fork: fork.c
	$(CC) $(CFLAGS) -o fork fork.c

thread: thread.c
	$(CC) $(CFLAGS) thread.c -o thread -lpthread

clean:
	rm -f numgen fork thread temp.txt
