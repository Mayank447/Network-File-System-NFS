#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <pthread.h>
#include <semaphore.h>

int socketID; //socketID for the server

/* Signal handler in case Ctrl-Z or Ctrl-D is pressed -> so that the socket gets closed */
void handle_signal(int signum) {
    close(socketID);
    exit(signum);
}

/* Function to close the socket*/
void closeSocket(){
    close(socketID);
    exit(1);
}

int main(int argc, char* argv[])
{   
    // Signal handler for Ctrl+C and Ctrl+Z
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    // Checking if the port number is provided
    if(argc!=2){
        printf("Usage: %s <port>\n", argv[0]);
        exit(1);
    }

    // Creating a socket
    int PORT = atoi(argv[1]);
    socketID = socket(AF_INET, SOCK_STREAM, 0);
    if(socketID < 0){
        perror("Error: opening socket\n");
        exit(1);
    }

    // Creating a server and client address structure
    struct sockaddr_in serverAddress, clientAddress;
    memset(&serverAddress, 0, sizeof(serverAddress));
    memset(&clientAddress, 0, sizeof(clientAddress));

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddress.sin_port = htons(PORT);

    // Binding the socket to the server address
    if(bind(socketID, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0){
        perror("Error: binding socket\n");
        exit(1);
    }
    printf("Storage Server has successfully started on port %d", PORT);

    // Closing the socket
    if(close(socketID) < 0){
        perror("Error: closing socket\n");
        exit(1);
    }
    return 0;
}
