CC = gcc
CFLAGS = -Wall

all: filegen schedtest

filegen: filegen.c
	$(CC) $(CFLAGS) filegen.c -o filegen

schedtest: schedtest.c
	$(CC) $(CFLAGS) schedtest.c -o schedtest

clean:
	rm -f filegen schedtest
