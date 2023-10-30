#include "header_files.h"
#include "name_server.h"

// int socketID; //socketID for the Name Server

// /* Signal handler in case Ctrl-Z or Ctrl-D is pressed -> so that the socket gets closed */
// void handle_signal(int signum) {
//     close(socketID);
//     exit(signum);
// }


// /* Function to close the socket*/
// void closeSocket(){
//     close(socketID);
//     exit(1);
// }

// int main(int argc, char* argv[])
// {   
//     // Signal handler for Ctrl+C and Ctrl+Z
//     signal(SIGINT, handle_signal);
//     signal(SIGTERM, handle_signal);

//     // Checking if the port number is provided
//     if(argc!=2){
//         printf("Usage: %s <port>\n", argv[0]);
//         exit(1);
//     }

//     // Creating a socket
//     int PORT = atoi(argv[1]);
//     if((socketID = socket(AF_INET, SOCK_STREAM, 0)) < 0){
//         perror("Error: opening socket\n");
//         exit(1);
//     }

//     // Creating a server and client address structure
//     struct sockaddr_in serverAddress, nameServerAddress, clientAddress;
//     memset(&serverAddress, 0, sizeof(serverAddress));
//     memset(&nameServerAddress, 0, sizeof(nameServerAddress));
//     memset(&clientAddress, 0, sizeof(clientAddress));

//     nameServerAddress.sin_family = AF_INET;
//     nameServerAddress.sin_addr.s_addr = htonl(INADDR_ANY);
//     nameServerAddress.sin_port = htons(PORT);

//     // Binding the socket to the server address
//     if(bind(socketID, (struct sockaddr*)&nameServerAddress, sizeof(nameServerAddress)) < 0){
//         perror("Error: binding socket\n");
//         exit(1);
//     }
//     printf("Name Server has successfully started on port %d", PORT);

//     // Sending the vital info to Name Server: IP Address, PORT FOR NS communication, PORT for SS communication, all accessible paths
    

//     // Closing the socket
//     if(close(socketID) < 0){
//         perror("Error: closing socket\n");
//         exit(1);
//     }
//     return 0;
// }



// Initialize a storage server
void initializeStorageServer(const char* nameServerIP, int nameServerPort) {
    // Create a socket for communication with the naming server
    int storageSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (storageSocket < 0) {
        perror("Error: opening storage socket\n");
        exit(1);
    }

    // Create a sockaddr_in structure for the naming server
    struct sockaddr_in nameServerAddr;
    memset(&nameServerAddr, 0, sizeof(nameServerAddr));
    nameServerAddr.sin_family = AF_INET;
    nameServerAddr.sin_port = htons(nameServerPort);
    if (inet_pton(AF_INET, nameServerIP, &nameServerAddr.sin_addr) <= 0) {
        perror("Error: invalid name server IP address\n");
        exit(1);
    }

    // Connect to the naming server
    if (connect(storageSocket, (struct sockaddr*)&nameServerAddr, sizeof(nameServerAddr)) < 0) {
        perror("Error: connecting to the naming server\n");
        exit(1);
    }

    // Construct a message with vital details for the current storage server
    char storageServerIP[16];
    int storageServerPort;
    // Extract storage server details (e.g., from user input)
    // For example:
    // sscanf(storageServerAddress, "%[^:]:%d", storageServerIP, &storageServerPort);

    char vitalDetailsMessage[1024];
    snprintf(vitalDetailsMessage, sizeof(vitalDetailsMessage), "IP: %s\nNM_PORT: %d\nCLIENT_PORT: %d\nPATHS: ...\n", storageServerIP, storageServerPort, 0);

    // Send vital details to the naming server
    if (send(storageSocket, vitalDetailsMessage, strlen(vitalDetailsMessage), 0) < 0) {
        perror("Error: sending vital details to naming server\n");
        exit(1);
    }

    // Close the socket when done
    close(storageSocket);
}

// Initialize a client
void initializeClient(const char* nameServerIP, int nameServerPort) {
    // Create a socket for communication with the naming server
    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket < 0) {
        perror("Error: opening client socket\n");
        exit(1);
    }

    // Create a sockaddr_in structure for the naming server
    struct sockaddr_in nameServerAddr;
    memset(&nameServerAddr, 0, sizeof(nameServerAddr));
    nameServerAddr.sin_family = AF_INET;
    nameServerAddr.sin_port = htons(nameServerPort);
    if (inet_pton(AF_INET, nameServerIP, &nameServerAddr.sin_addr) <= 0) {
        perror("Error: invalid name server IP address\n");
        exit(1);
    }

    // Connect to the naming server
    if (connect(clientSocket, (struct sockaddr*)&nameServerAddr, sizeof(nameServerAddr)) < 0) {
        perror("Error: connecting to the naming server\n");
        exit(1);
    }

    // Construct a message with vital details for the current client
    char clientIP[16];
    int clientPort;
    // Extract client details (e.g., from user input)
    // For example:
    // sscanf(clientAddress, "%[^:]:%d", clientIP, &clientPort);

    char vitalDetailsMessage[1024];
    snprintf(vitalDetailsMessage, sizeof(vitalDetailsMessage), "IP: %s\nPORT: %d\n", clientIP, clientPort);

    // Send vital details to the naming server
    if (send(clientSocket, vitalDetailsMessage, strlen(vitalDetailsMessage), 0) < 0) {
        perror("Error: sending vital details to naming server\n");
        exit(1);
    }

    // Close the socket when done
    close(clientSocket);
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Usage: %s <name_server_ip:port> <server/client>\n", argv[0]);
        exit(1);
    }

    char* nameServerAddress = argv[1]; // Format: "name_server_ip:name_server_port"
    int nameServerPort = 0;
    sscanf(nameServerAddress, "%*[^:]:%d", &nameServerPort);

    char* type = argv[2];

    if (strcmp(type, "server") == 0) {
        initializeStorageServer(nameServerAddress, nameServerPort);
    } else if (strcmp(type, "client") == 0) {
        initializeClient(nameServerAddress, nameServerPort);
    } else {
        printf("Invalid type. Use 'server' or 'client'.\n");
    }

    return 0;
}
