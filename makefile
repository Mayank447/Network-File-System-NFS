CC = gcc
CFLAGS = -Wall -std=c99

all: client storage_server name_server

helper_functions.o: helper_functions.c
		$(CC) $(CFLAGS) -O -c helper_functions.c

client: client.c 
		$(CC) $(CFLAGS) -o client client.c

storage_server: storage_server.c helper_functions.c
		$(CC) $(CFLAGS) -o storage_server storage_server.c

name_server: name_server.c helper_functions.o
		$(CC) $(CFLAGS) -o name_server name_server.c helper_functions.o

clean:
	rm -f client storage_server name_server		