CC = gcc
CFLAGS = -Wall -std=c99

all: client storage_server name_server

client: client.c helper_functions.c
		$(CC) $(CFLAGS) -o client client.c helper_functions.c

storage_server: storage_server.c helper_functions.c
		$(CC) $(CFLAGS) -o storage_server storage_server.c helper_functions.c

name_server: name_server.c helper_functions.c
		$(CC) $(CFLAGS) -o name_server name_server.c helper_functions.c

clean:
	rm -f client storage_server name_server		