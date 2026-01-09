CC=gcc
CFLAGS=-Wall -pthread

all: server client

server:
	$(CC) $(CFLAGS) src/server/server.c -o server

client:
	$(CC) $(CFLAGS) src/client/client.c -o client

clean:
	rm -f server client