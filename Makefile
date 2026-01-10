CC=gcc
CFLAGS=-Wall -pthread

all: server client

server:
	gcc -Wall -pthread src/server/server.c \
	-I/usr/local/include \
	-L/usr/local/lib -lssl -lcrypto \
	-o server

client:
	$(CC) $(CFLAGS) src/client/client.c -o client

clean:
	rm -f server client