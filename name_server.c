// #include "header_files.h"
// #include "name_server.h"

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



// // Initialize a storage server
// void initializeStorageServer(const char* nameServerIP, int nameServerPort) {
//     // Create a socket for communication with the naming server
//     int storageSocket = socket(AF_INET, SOCK_STREAM, 0);
//     if (storageSocket < 0) {
//         perror("Error: opening storage socket\n");
//         exit(1);
//     }

//     // Create a sockaddr_in structure for the naming server
//     struct sockaddr_in nameServerAddr;
//     memset(&nameServerAddr, 0, sizeof(nameServerAddr));
//     nameServerAddr.sin_family = AF_INET;
//     nameServerAddr.sin_port = htons(nameServerPort);
//     if (inet_pton(AF_INET, nameServerIP, &nameServerAddr.sin_addr) <= 0) {
//         perror("Error: invalid name server IP address\n");
//         exit(1);
//     }

//     // Connect to the naming server
//     if (connect(storageSocket, (struct sockaddr*)&nameServerAddr, sizeof(nameServerAddr)) < 0) {
//         perror("Error: connecting to the naming server\n");
//         exit(1);
//     }

//     // Construct a message with vital details for the current storage server
//     char storageServerIP[16];
//     int storageServerPort;
//     // Extract storage server details (e.g., from user input)
//     // For example:
//     // sscanf(storageServerAddress, "%[^:]:%d", storageServerIP, &storageServerPort);

//     char vitalDetailsMessage[1024];
//     snprintf(vitalDetailsMessage, sizeof(vitalDetailsMessage), "IP: %s\nNM_PORT: %d\nCLIENT_PORT: %d\nPATHS: ...\n", storageServerIP, storageServerPort, 0);

//     // Send vital details to the naming server
//     if (send(storageSocket, vitalDetailsMessage, strlen(vitalDetailsMessage), 0) < 0) {
//         perror("Error: sending vital details to naming server\n");
//         exit(1);
//     }

//     // Close the socket when done
//     close(storageSocket);
// }

// // Initialize a client
// void initializeClient(const char* nameServerIP, int nameServerPort) {
//     // Create a socket for communication with the naming server
//     int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
//     if (clientSocket < 0) {
//         perror("Error: opening client socket\n");
//         exit(1);
//     }

//     // Create a sockaddr_in structure for the naming server
//     struct sockaddr_in nameServerAddr;
//     memset(&nameServerAddr, 0, sizeof(nameServerAddr));
//     nameServerAddr.sin_family = AF_INET;
//     nameServerAddr.sin_port = htons(nameServerPort);
//     if (inet_pton(AF_INET, nameServerIP, &nameServerAddr.sin_addr) <= 0) {
//         perror("Error: invalid name server IP address\n");
//         exit(1);
//     }

//     // Connect to the naming server
//     if (connect(clientSocket, (struct sockaddr*)&nameServerAddr, sizeof(nameServerAddr)) < 0) {
//         perror("Error: connecting to the naming server\n");
//         exit(1);
//     }

//     // Construct a message with vital details for the current client
//     char clientIP[16];
//     int clientPort;
//     // Extract client details (e.g., from user input)
//     // For example:
//     // sscanf(clientAddress, "%[^:]:%d", clientIP, &clientPort);

//     char vitalDetailsMessage[1024];
//     snprintf(vitalDetailsMessage, sizeof(vitalDetailsMessage), "IP: %s\nPORT: %d\n", clientIP, clientPort);

//     // Send vital details to the naming server
//     if (send(clientSocket, vitalDetailsMessage, strlen(vitalDetailsMessage), 0) < 0) {
//         perror("Error: sending vital details to naming server\n");
//         exit(1);
//     }

//     // Close the socket when done
//     close(clientSocket);
// }

// int main(int argc, char *argv[]) {
//     if (argc < 3) {
//         printf("Usage: %s <name_server_ip:port> <server/client>\n", argv[0]);
//         exit(1);
//     }

//     char* nameServerAddress = argv[1]; // Format: "name_server_ip:name_server_port"
//     int nameServerPort = 0;
//     sscanf(nameServerAddress, "%*[^:]:%d", &nameServerPort);

//     char* type = argv[2];

//     if (strcmp(type, "server") == 0) {
//         initializeStorageServer(nameServerAddress, nameServerPort);
//     } else if (strcmp(type, "client") == 0) {
//         initializeClient(nameServerAddress, nameServerPort);
//     } else {
//         printf("Invalid type. Use 'server' or 'client'.\n");
//     }

//     return 0;
// }



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#define CLIENT_PORT 8080
#define STORAGE_SERVER_PORT 9090

int ss_count = 0;

// Data structure to represent file information
struct StorageServerInfo {
    int ss_id;
    char ip_address[256];
    int naming_server_port;
    int client_server_port;
    char accessible_paths[300][300];
    struct StorageServerInfo* next;
};

struct StorageServerInfo* storageServerList = NULL;

// Mutex for protecting the fileTable
pthread_mutex_t fileTableMutex = PTHREAD_MUTEX_INITIALIZER;

// Mutex for controlling server initialization
pthread_mutex_t serverInitMutex = PTHREAD_MUTEX_INITIALIZER;

// Sockets for clients and storage servers
int clientServerSocket, storageServerSocket;

// Function to search for a path in storage servers
int searchStorageServer(const char* file_path, struct StorageServerInfo* ss) {
    int found = 0;
    struct StorageServerInfo* temp = ss;

    while (temp != NULL) {
        // Search accessible paths in the current storage server
        int i = 0;
        while (temp->accessible_paths[i][0] != '\0') {  // Check if the path is not an empty string
            if (strcmp(file_path, temp->accessible_paths[i]) == 0) {
                found = 1;
                break; // Path found, no need to continue searching
            }
            i++;
        }

        if (found) {
            break; // Path found in this storage server, exit the loop
        }

        temp = temp->next;
    }

    if (found) {
        return 0; // Found the path in a storage server
    } else {
        return -1; // Path not found in any storage server
    }
}

// Data structure for hash based storage
struct HashMap {
    char* file_path;
    struct StorageServerInfo* storage_server;
    struct HashMap* next;
};

// Function to create a new HashMap entry
struct HashMap* createHashMapEntry(const char* file_path, struct StorageServerInfo* ss) {
    struct HashMap* entry = (struct HashMap*)malloc(sizeof(struct HashMap));
    entry->file_path = strdup(file_path);
    entry->storage_server = ss;
    entry->next = NULL;
    return entry;
}

// Function to perform a fast lookup in the HashMap
struct StorageServerInfo* efficientStorageServerSearch(struct HashMap* hashmap, const char* file_path) {
    struct HashMap* current = hashmap;
    while (current != NULL) {
        if (strcmp(current->file_path, file_path) == 0) {
            return current->storage_server;
        }
        current = current->next;
    }
    return NULL; // File path not found in the HashMap
}

// Function to add storage server information to the linked list
void addStorageServerInfo(const char* ip, int ns_port, int cs_port) {
    struct StorageServerInfo* newServer = (struct StorageServerInfo*)malloc(sizeof(struct StorageServerInfo));
    strcpy(newServer->ip_address, ip);
    newServer->naming_server_port = ns_port;
    newServer->client_server_port = cs_port;
    newServer->ss_id = ++ss_count;
    newServer->next = storageServerList;
    for(int i = 0; i < 300; i++)
    {
        newServer->accessible_paths[newServer->ss_id][i] = NULL;
    }
    storageServerList = newServer;
}

// Function to handle client requests
void* handleClientRequests(void* arg) {
    int new_socket;
    struct sockaddr_in new_addr;
    socklen_t addr_size;
    char buffer[1024];
    int bytesReceived;

    while (1) {
        addr_size = sizeof(new_addr);
        new_socket = accept(clientServerSocket, (struct sockaddr*)&new_addr, &addr_size); // Accept a client connection

        // Handle client requests here
        // ...

        close(new_socket);
    }
}

// Function to handle storage server queries
void parseStorageServerInfo(const char* data, char* ip_address, int* ns_port, int* cs_port) {
    // Implement a parsing logic based on your message format
    // For example, if your message format is "IP:PORT1:PORT2", you can use sscanf
    if (sscanf(data, "%[^:]:%d:%d", ip_address, ns_port, cs_port) != 3) {
        fprintf(stderr, "Error parsing storage server info: %s\n", data);
    }
}

void* handleStorageServerQueries(void* arg) {
    struct sockaddr_in server_addr, new_addr;
    socklen_t addr_size;
    char buffer[1024];
    int bytesReceived;

    while (1) {
        addr_size = sizeof(new_addr);
        int new_socket = accept(storageServerSocket, (struct sockaddr*)&new_addr, &addr_size); // Accept a storage server connection

        // Handle storage server queries here
        while (1) {
            bytesReceived = recv(new_socket, buffer, 1024, 0);
            if (bytesReceived < 0) {
                perror("Error in receiving data");
                exit(1);
            }
            buffer[bytesReceived] = '\0';

            // Check if the received message is "NEW STORAGE SERVER CREATED"
            if (strcmp(buffer, "NEW STORAGE SERVER CREATED") == 0) {
                // Receive additional information from the storage server
                char ip_address[256];
                int naming_server_port, client_server_port;

                bytesReceived = recv(new_socket, buffer, 1024, 0);
                if (bytesReceived < 0) {
                    perror("Error in receiving data");
                    exit(1);
                }
                buffer[bytesReceived] = '\0';

                // Parse the received information
                parseStorageServerInfo(buffer, ip_address, &naming_server_port, &client_server_port);

                // Add the information to the linked list
                addStorageServerInfo(ip_address, naming_server_port, client_server_port);
            }
        }

        close(new_socket);
    }
}

// Function to close server connections
void closeConnections() {
    close(clientServerSocket);
    close(storageServerSocket);
}

int main() {
    initializeFileTable();

    pthread_t clientThread, storageServerThread;

    // Initialize server sockets
    clientServerSocket = socket(AF_INET, SOCK_STREAM, 0);
    storageServerSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientServerSocket < 0 || storageServerSocket < 0) {
        perror("Error in socket");
        exit(1);
    }

    struct sockaddr_in clientServerAddr, storageServerAddr;
    clientServerAddr.sin_family = AF_INET;
    clientServerAddr.sin_port = htons(CLIENT_PORT);
    clientServerAddr.sin_addr.s_addr = INADDR_ANY;
    storageServerAddr.sin_family = AF_INET;
    storageServerAddr.sin_port = htons(STORAGE_SERVER_PORT);
    storageServerAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(clientServerSocket, (struct sockaddr*)&clientServerAddr, sizeof(clientServerAddr)) < 0 ||
        bind(storageServerSocket, (struct sockaddr*)&storageServerAddr, sizeof(storageServerAddr)) < 0) {
        perror("Error in binding");
        exit(1);
    }

    if (listen(clientServerSocket, 10) == 0 && listen(storageServerSocket, 10) == 0) {
        printf("Listening for client requests and storage server queries...\n");
    } else {
        perror("Error in listening");
        exit(1);
    }

    pthread_create(&clientThread, NULL, handleClientRequests, NULL);
    pthread_create(&storageServerThread, NULL, handleStorageServerQueries, NULL);

    // Wait for user input to close connections
    printf("Press Enter to close server connections...\n");
    getchar();

    // Close server connections
    closeConnections();

    // Wait for threads to finish
    pthread_join(clientThread, NULL);
    pthread_join(storageServerThread, NULL);

    return 0;
}



