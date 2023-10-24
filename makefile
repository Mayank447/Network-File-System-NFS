CC = gcc
CFLAGS = -Wall -std=c99

all: client storage_server name_server

client: client.c
		$(CC) $(CFLAGS) -o client client.c

storage_server: storage_server.c
		$(CC) $(CFLAGS) -o storage_server storage_server.c

name_server: name_server.c
		$(CC) $(CFLAGS) -o name_server name_server.c

clean:
	rm -f client storage_server name_server		