#include "name_server.h"

int socketID; //socketID for the Name Server

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
    if((socketID = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        perror("Error: opening socket\n");
        exit(1);
    }

    // Creating a server and client address structure
    struct sockaddr_in serverAddress, nameServerAddress, clientAddress;
    memset(&serverAddress, 0, sizeof(serverAddress));
    memset(&nameServerAddress, 0, sizeof(nameServerAddress));
    memset(&clientAddress, 0, sizeof(clientAddress));

    nameServerAddress.sin_family = AF_INET;
    nameServerAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    nameServerAddress.sin_port = htons(PORT);

    // Binding the socket to the server address
    if(bind(socketID, (struct sockaddr*)&nameServerAddress, sizeof(nameServerAddress)) < 0){
        perror("Error: binding socket\n");
        exit(1);
    }
    printf("Name Server has successfully started on port %d", PORT);

    // Sending the vital info to Name Server: IP Address, PORT FOR NS communication, PORT for SS communication, all accessible paths


    // Closing the socket
    if(close(socketID) < 0){
        perror("Error: closing socket\n");
        exit(1);
    }
    return 0;
}