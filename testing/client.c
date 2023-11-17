#include "header_files.h"
#include "params.h"
#include "client.h"
#include "helper_functions.h"

#define NAMESERVER_PORT 8080  // PORT for communication with nameserver (fixed)
#define ERROR_BUFFER_LENGTH 1024
#define BUFFER_SIZE 1024

char error_message[ERROR_BUFFER_LENGTH];

///////////////////////// USER INPUT OPERATION //////////////////////////

/* Function to print all the possible operations performable*/
void printOperations(){
    printf("1. Create a File\n");
    printf("2. Create Folder\n");
    printf("3. Read File\n");
    printf("4. Write to a File\n");
    printf("5. Obtain the metadata about a File\n");
    printf("6. Copy File\n"); // Specify the two directory paths
    printf("6. Delete File\n");
    printf("8. Copy Folder\n"); // Specify the two paths
    printf("9. Delete Folder\n");
    printf("Choose anoperation: ");
}

// Function to get the file path from the user
void getFilePath(char* path1){
    printf("Enter the file path: ");
    scanf("%s", path1);
}


///////////////////////// FETCHING AND CONNECTING TO STORAGE SERVER //////////////////
void handleErrorCodes(int valid, char* message){
    if(valid == 0) 
        strcpy(message, "VALID");
    else if(valid == 1)
        strcpy(message, "NAMESERVER ERROR");
    else if(valid == 2) 
        strcpy(message, "PATH DOES NOT EXIST");
    else if(valid == 3)
        strcpy(message, "STORAGE SERVER IS DOWN");
    else if(valid == 4)
        strcpy(message, "A CLIENT IS ALREADY READING THE FILE");
    else if(valid == 5)
        strcpy(message, "A CLIENT IS ALREADY WRITING TO THE FILE");
    else if(valid == 6)
        strcpy(message, "FILE DOES NOT EXIST");
    else if(valid == 7)
        strcpy(message, "FILE ALREADY EXISTS");
    else if(valid == 8)
        strcpy(message, "DIRECTORY DOES NOT EXIST");
    else if(valid == 9)
        strcpy(message, "DIRECTORY ALREADY EXIST");
    else if(valid == 10)
        strcpy(message, "INVALID OPERATION NUMBER");
}

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
    close(nsSocket);
    return parseIpPort(buffer, IP_address, PORT);
}

// IP address parsing logic based on message format "IP:PORT"
int parseIpPort(char *data, char *ip_address,int *ss_port)
{
    if(strcmp(data, "2")==0) {
        printf("INVALID FILE PATH\n");
        return -1;
    }
    if (sscanf(data, "%[^:]:%d", ip_address, ss_port) != 2){
        printf("Error parsing storage server info: %s\n", data);
        return -1;
    }
    return 0;
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

// Sending the operation number to the connected Storage Server
int sendOperationNumber_Path(int serverSocket, char* operation_no_path)
{
    // Sending the operation number
    if(send(serverSocket, operation_no_path, strlen(operation_no_path), 0) < 0){
        perror("Error sendOperationNumber_Path(): Unable to send the OPERATION NUMBER to the server");
        close(serverSocket);
        return -1;
    }

    // Checking if the oepration number is valid
    char reply[1000];
    if(recv(serverSocket, reply, sizeof(reply), 0) < 0){
        perror("Error sendOperationNumber_Path(): Unable to receive the acknowledgement for OPERATION_NUMBER/PATH sent to the server");
        close(serverSocket);
        return -1;
    }

    if(strcmp(reply, "0")!=0) {
        bzero(error_message, ERROR_BUFFER_LENGTH);
        handleErrorCodes(atoi(reply), error_message);
        printf("Error sendOperationNumber_Path(): %s\n", error_message);
        close(serverSocket);
        return -1;
    }
    return 0;
}


////////////////////////////// FILE OPERATION /////////////////////////////

void createFile(char* path){
    
}

void readFile(char* path) // Function to download the specified file
{
    int PORT = 0;
    char IP_address[20]; 
    if(fetchStorageServerIP_Port(path, IP_address, &PORT) < 0) {
        return;
    }
    int serverSocket = connectToStorageServer(IP_address, PORT);
    
    if(sendOperationNumber_Path(serverSocket, "3") == -1) return;
    if(sendOperationNumber_Path(serverSocket, path) == -1) return;

    // Receiving the file
    char filename[MAX_FILE_NAME_LENGTH];
    extractFileName(path, filename);
    downloadFile(serverSocket, filename);
    close(serverSocket);
}


void writeToFile(char* path) // Function to upload a file to the server
{
    int PORT = 0;
    char IP_address[20]; 
    if(fetchStorageServerIP_Port(path, IP_address, &PORT) < 0) {
        return;
    }

    int serverSocket = connectToStorageServer(IP_address, PORT);
    if(sendOperationNumber_Path(serverSocket, "4") == -1) return;
    if(sendOperationNumber_Path(serverSocket, path) == -1) return;

    printf("uploadFile successful");
    close(serverSocket);
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

        // Creating a file specified a file path
        if(op == 1){
            getFilePath(path1);
            createFile(path1);
        }

        // Reading file from a specified file path
        else if(op == 3) {
            getFilePath(path1);
            readFile(path1);    
        }

        // Writing (Uploading file) to server
        else if(op == 4){
            getFilePath(path1);
            char content[10000];
            printf("Enter the contents of the File: ");
            fgets(content, 10000, stdin);
            writeToFile(path1);
        }
    }

    /*
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
    return 0;
}