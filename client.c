#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdio.h>
#include "params.h"
#include "client.h"

#define NAMESERVER_PORT 8080  // PORT for communication with nameserver (fixed)
#define BUFFER_SIZE 1024

///////////////////////// USER INPUT OPERATION //////////////////////////

/* Function to print all the possible operations performable*/
void printOperations(){
    printf("1. Read File\n");
    printf("2. Write to a File\n");
    printf("3. Obtain the metadata about a File\n");
    printf("4. Create a File\n");
    printf("5. Copy File\n"); // Specify the two directory paths
    printf("6. Delete File\n");
    printf("7. Create Folder\n");
    printf("8. Copy Folder\n"); // Specify the two paths
    printf("9. Delete Folder\n");
    printf("Choose operation:");
}

// Function to get the file path from the user
void getFilePath(char* path1){
    printf("Enter the file path: ");
    scanf("%s", path1);
}


///////////////////////// FETCHING AND CONNECTING TO STORAGE SERVER //////////////////

/* Function to fetch the Storage Server IP and PORT given the path*/
int fetchStorageServerIP_Port(const char* path, char* IP_address, int* PORT)
{   
    int nsSocket; // Socket from client side to name-server
    struct sockaddr_in name_server_address;
    char buffer[BUFFER_SIZE];

    // Opening a socket for connection with the nameserver
    if ((nsSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation failed");
        return -1;
    }
    
    name_server_address.sin_family = AF_INET;
    name_server_address.sin_port = htons(NAMESERVER_PORT);
    name_server_address.sin_addr.s_addr = INADDR_ANY;
    if (inet_pton(AF_INET, "127.0.0.1", &name_server_address.sin_addr) < 0){
        perror("Invalid address/Address not supported");
        close(nsSocket);
        return -1;
    }

    if (connect(nsSocket, (struct sockaddr *)&name_server_address, sizeof(name_server_address)) < 0){
        perror("Connection to nameserver failed");
        close(nsSocket);
        return -1;
    }
    printf("Connected to Nameserver..\n");

    if(send(nsSocket, path, strlen(path), 0) < 0){
        perror("Error fetchStorageServerIP_Port(): Unable to send specified path");
        close(nsSocket);
        return -1;
    }

    if(recv(nsSocket, buffer, BUFFER_SIZE, 0) < 0){
        perror("Error fetchStorageServerIP_Port(): Unable to receive the requested Storage server IP and PORT.");
        close(nsSocket);
        return -1;
    }

    parseIpPort(buffer, IP_address, PORT);
    close(nsSocket);
    return 0;
}

// IP address parsing logic based on message format "IP:PORT"
void parseIpPort(char *data, char *ip_address,int *ss_port)
{
    if (sscanf(data, "%[^:]:%d", ip_address, ss_port) != 2){
        printf("Error parsing storage server info: %s\n", data);
    }
}

/* Connect to the storage server given the IP and PORT */
int connectToStorageServer(const char* IP_address, const int PORT)
{
    int serverSocket;
    if((serverSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        perror("Error connectToStorageServer(): Connecting to Storage server");
        return -1;
    }

    struct sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(PORT);
    serverAddress.sin_addr.s_addr = inet_addr(IP_address);
        
    if (connect(serverSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0){
        perror("Connection to new server failed");
        return -1;
    }
    return serverSocket;
}



////////////////////////////// FILE OPERATION /////////////////////////////

// Read file
void readFile(char* path)
{
    char IP_address[20]; int PORT = 0;
    if(fetchStorageServerIP_Port(path, IP_address, &PORT) < 0) {
        return;
    }

    int serverSocket = connectToStorageServer(IP_address, PORT);
    printf("getFile successful");
}

void uploadFile(char* path){

}

// To the formatted requested meta data for a file
void parseMetadata(const char *data, char *filepath, int *size, int *permissions) {
    char *token;
    char copy[1024];
    strcpy(copy, data); // Create a copy because strtok modifies the string

    token = strtok(copy, ":");
    if (token != NULL) {
        strcpy(filepath, token);
        token = strtok(NULL, ":");
        if (token != NULL) {
            *size = atoi(token);
            token = strtok(NULL, ":");
            if (token != NULL) {
                *permissions = atoi(token);
            } else {
                printf("Error parsing permissions: %s\n", data);
            }
        } else {
            printf("Error parsing size: %s\n", data);
        }
    } else {
        printf("Error parsing filepath: %s\n", data);
    }
}



////////////////////////////// DIRECTORY OPERATION ////////////////////////



////////////////////////////// MAIN FUNCTION /////////////////////////////////
int main()
{
    int op = -1;
    char path1[MAX_PATH_LENGTH], path2[MAX_PATH_LENGTH];

    while(op != 0){
        printOperations();
        scanf("%d",&op);

        bzero(path1, MAX_PATH_LENGTH);
        bzero(path2, MAX_PATH_LENGTH);

        // Reading file from a specified file path
        if(op == 1) {
            getFilePath(path1);
            readFile(path1);    
        }

        // Writing (Uploading file) to server
        else if(op == 2){
            printf("Specify the file path which you want to upload: ");
            scanf("%s", path1);
            uploadFile(path1);
        }
    }



    /*
    if (operation==1)
    {
        int o=1;


        // int new_server_port1=atoi(new_server_port);
        
        // int new_socket = accept(new_client_socket, (struct sockaddr*)&new_server_address, &new_server_address);
        printf("hello\n");
        int a=1;
        if (send(new_client_socket, &a, sizeof(a), 0) < 0)
        {
            perror("Error: sending completed message to Naming Server\n");
            close(new_client_socket);
            return -1;
        }
        printf("hello %s\n", file_path);
        if (send(new_client_socket, file_path, strlen(file_path), 0) < 0)
        {
            perror("Error: sending completed message to Naming Server\n");
            close(new_client_socket);
            return -1;
        }
        printf("hello again\n");
        char buffer[1024]={'\0'}; // Adjust the buffer size as needed
        ssize_t bytes_received;
        while (1)
        {
            // Print the received data
            if ((bytes_received = recv(new_client_socket, buffer, sizeof(buffer), 0)) < 0)
            {
                perror("Receive error");
                close(new_client_socket);
                exit(1);
            }
            else
            {
                if(strcmp(buffer,"STOP")==0){
                    break;
                }
                printf("%s",buffer);
            }
        }
        printf("ABOVE INFORMATION IS PRESENT IN FILE\n");
        // Receive the content from the socket and print it on the terminal
        close(new_client_socket);
    }
    //Closing Storage server
    else if (operation == 2) {
        int o=1;

        // Perform read operation
        printf("Enter the file path to write: \n");

        char file_path[256];
        scanf("%s",file_path);

        if (send(clientSocketID, &o, sizeof(o), 0) < 0)
        {
            perror("Error: sending completed message to Naming Server\n");
            close(clientSocketID);
            return -1;
        }

        // Send the file path to the naming server
        if(send(clientSocketID, file_path, strlen(file_path), 0) < 0)
        {
            perror("Error: sending file path to Naming Server\n");
            close(clientSocketID);
            return -1;
        }
        
        // Receive IP address and port of the storage server (SS) from the naming server
        char store[1024]={'\0'};
        if(recv(clientSocketID, store, sizeof(store), 0) < 0)
        {
            perror("Error: receiving new server IP and Port from Naming Server\n");
            close(clientSocketID);
            return -1;
        }

        printf("%s\n",store);

        // parse the ip address port number with delimiter a :
        parseIpPort(store, NameServerIP, &new_server_port);

        int new_client_socket;
        struct sockaddr_in new_server_address;
        if ((new_client_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        {
            perror("Socket creation failed");
            exit(EXIT_FAILURE);
        }

        new_server_address.sin_family = AF_INET;
        new_server_address.sin_port = htons(new_server_port); // new_server_port1);new_server_ip
        new_server_address.sin_addr.s_addr = INADDR_ANY;
        if (inet_pton(AF_INET, "127.0.0.1", &new_server_address.sin_addr) < 0)
        {
            perror("Invalid address/Address not supported");
            exit(EXIT_FAILURE);
        }
        if (connect(new_client_socket, (struct sockaddr *)&new_server_address, sizeof(new_server_address)) < 0)
        {
            perror("Connection to new server failed");
            exit(EXIT_FAILURE);
        }

        int a=2;

        if (send(new_client_socket, &a, sizeof(a), 0) < 0)
        {
            perror("Error: sending completed message to Naming Server\n");
            close(new_client_socket);
            return -1;
        }

        if (send(new_client_socket, file_path, strlen(file_path), 0) < 0)
        {
            perror("Error: sending completed message to Naming Server\n");
            close(new_client_socket);
            return -1;
        }

        char buffer[1024]={'\0'}; // Adjust the buffer size as needed
        ssize_t bytes_received;
        printf("Enter data to add to file: \n");

        char input_data[10000];

        while(1)
        {
            scanf("%[^\n]s",input_data);

            // send data to append to file
            if(send(new_client_socket, input_data, strlen(input_data), 0) < 0)
            {
                perror("Send error");
                close(new_client_socket);
                exit(1);
            }

            if(strcmp(input_data, "STOP") == 0)
            {
                break;
            }
        }
        
        // Print the received data
        if ((bytes_received = recv(new_client_socket, buffer, sizeof(buffer), 0)) < 0)
        {
            perror("Receive error");
            close(new_client_socket);
            exit(1);
        }

        printf("%s\n", buffer);
        printf("ABOVE INFORMATION IS WRITTEN TO THE FILE\n");
        // Receive the content from the socket and print it on the terminal
        close(new_client_socket);
        
    } else if (operation == 3) {
        // Perform file information operation
        int o=1;

        // Perform read operation
        printf("Enter the file to get info: \n");

        char file_path[256];
        scanf("%s",file_path);

        if (send(clientSocketID, &o, sizeof(o), 0) < 0)
        {
            perror("Error: sending completed message to Naming Server\n");
            close(clientSocketID);
            return -1;
        }

        // Send the file path to the naming server
        if(send(clientSocketID, file_path, strlen(file_path), 0) < 0)
        {
            perror("Error: sending file path to Naming Server\n");
            close(clientSocketID);
            return -1;
        }
        
        // Receive IP address and port of the storage server (SS) from the naming server
        char store[1024]={'\0'};
        if(recv(clientSocketID, store, sizeof(store), 0) < 0)
        {
            perror("Error: receiving new server IP and Port from Naming Server\n");
            close(clientSocketID);
            return -1;
        }

        printf("%s\n",store);

        // parse the ip address port number with delimiter a :
        parseIpPort(store, new_server_ip, &new_server_port);

        int new_client_socket;
        struct sockaddr_in new_server_address;
        if ((new_client_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        {
            perror("Socket creation failed");
            exit(EXIT_FAILURE);
        }

        new_server_address.sin_family = AF_INET;
        new_server_address.sin_port = htons(new_server_port); // new_server_port1);new_server_ip
        new_server_address.sin_addr.s_addr = INADDR_ANY;
        if (inet_pton(AF_INET, "127.0.0.1", &new_server_address.sin_addr) < 0)
        {
            perror("Invalid address/Address not supported");
            exit(EXIT_FAILURE);
        }
        if (connect(new_client_socket, (struct sockaddr *)&new_server_address, sizeof(new_server_address)) < 0)
        {
            perror("Connection to new server failed");
            exit(EXIT_FAILURE);
        }

        int a=3;

        if (send(new_client_socket, &a, sizeof(a), 0) < 0)
        {
            perror("Error: sending completed message to Naming Server\n");
            close(new_client_socket);
            return -1;
        }

        if (send(new_client_socket, file_path, strlen(file_path), 0) < 0)
        {
            perror("Error: sending completed message to Naming Server\n");
            close(new_client_socket);
            return -1;
        }

        char buffer[1024]={'\0'}; // Adjust the buffer size as needed
        ssize_t bytes_received;
        printf("File meta data: \n");
        char  file_name[256];
        int  file_size,  file_permissions;    

        // Print the received data
        if ((bytes_received = recv(new_client_socket, buffer, sizeof(buffer), 0)) < 0)
        {
            perror("Receive error");
            close(new_client_socket);
            exit(1);
        }
        printf("%s\n", buffer);
        parseMetadata(buffer, file_name, &file_size, &file_permissions);
        printf("file name:%s\n",file_name);
        printf("file size:%d\n",file_size);
        printf("file permission:%d\n",file_permissions);
        printf("ABOVE INFORMATION IS META DATA OF THE FILE\n");
        // Receive the content from the socket and print it on the terminal
        close(new_client_socket);
        
    } else {
        printf("Invalid operation choice.\n");
    }
    */

    // Do not close client_socket here
    // close(client_socket);
    return 0;
}