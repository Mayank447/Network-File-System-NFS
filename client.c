#include "header_files.h"
#include "client.h"

#define NAMESERVER_PORT 8080  // PORT for communication with nameserver (fixed)

char error_message[ERROR_BUFFER_LENGTH];

///////////////////////// USER INPUT OPERATION //////////////////////////

/* Function to print all the possible operations performable*/
void printOperations(){
    printf("\n1. Create a File\n");
    printf("2. Create Folder\n");
    printf("3. Read File\n");
    printf("4. Write to a File\n");
    printf("5. Obtain the metadata about a File\n");
    printf("6. Delete File\n");
    printf("7. Delete Folder\n");
    printf("8. Copy File\n"); // Specify the two directory paths
    printf("9. Copy Folder\n"); // Specify the two paths
    printf("Choose an operation: ");
    fflush(stdin);
}

// Function to get the file path from the user
void getFilePath(char* path){
    printf("Enter the file path: ");
    scanf("%s", path);
}

// Function to get the directory path from the user
void getFirectoryPath(char* path){
    printf("Enter the directory path: ");
    scanf("%s", path);
}


///////////////////////// FETCHING AND CONNECTING TO STORAGE SERVER //////////////////
int sendCommandToNameServer(char* operation_num, char* path)
{
    int nsSocket; // Socket from client side to name-server
    struct sockaddr_in name_server_address;

    // Opening a socket for connection with the nameserver
    if ((nsSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("[-] Error sendCommandToNameServer(): Socket creation failed");
        return -1;
    }
    
    name_server_address.sin_family = AF_INET;
    name_server_address.sin_port = htons(NAMESERVER_PORT);
    name_server_address.sin_addr.s_addr = INADDR_ANY;
    
    if (inet_pton(AF_INET, "127.0.0.1", &name_server_address.sin_addr) < 0){
        perror("[-] Error sendCommandToNameServer(): Invalid address/Address not supported");
        close(nsSocket);
        return -1;
    }

    // Connecting to the nameserver
    if (connect(nsSocket, (struct sockaddr *)&name_server_address, sizeof(name_server_address)) < 0){
        perror("[-] Error sendCommandToNameServer(): Connection to nameserver failed");
        close(nsSocket);
        return -1;
    }
    printf("[+] Connected to Nameserver..\n");

    // Sending the Operation number to the Name server
    if(send(nsSocket, operation_num, strlen(operation_num), 0) < 0){
        perror("[-] Error sendCommandToNameServer(): Unable to send operation number to the Nameserver");
        close(nsSocket);
        return -1;
    }

    // Receiving the Operation Number Confirmation from the Nameserver and Checking if the operation is valid
    if(receiveConfirmation(nsSocket)){
        perror("[-] Error sendCommandToNameServer(): Unable to receive confirmation from the Nameserver");
        close(nsSocket);
        return -1;
    } 
    
    return nsSocket;
}


int fetchStorageServerIP_Port(const char* path, char* IP_address, int* PORT)
{   
    int nsSocket; // Socket from client side to name-server
    struct sockaddr_in name_server_address;
    char buffer[BUFFER_LENGTH];

    // Opening a socket for connection with the nameserver
    if ((nsSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("[-] Socket creation failed");
        return -1;
    }
    
    name_server_address.sin_family = AF_INET;
    name_server_address.sin_port = htons(NAMESERVER_PORT);
    name_server_address.sin_addr.s_addr = INADDR_ANY;
    
    if (inet_pton(AF_INET, "127.0.0.1", &name_server_address.sin_addr) < 0){
        perror("[-] Invalid address/Address not supported");
        close(nsSocket);
        return -1;
    }

    if (connect(nsSocket, (struct sockaddr *)&name_server_address, sizeof(name_server_address)) < 0){
        perror("[-] Connection to nameserver failed");
        close(nsSocket);
        return -1;
    }
    printf("[+] Connected to Nameserver..\n");

    if(send(nsSocket, path, strlen(path), 0) < 0){
        perror("[-] Error fetchStorageServerIP_Port(): Unable to send specified path");
        close(nsSocket);
        return -1;
    }

    if(recv(nsSocket, buffer, BUFFER_LENGTH, 0) < 0){
        perror("[-] Error fetchStorageServerIP_Port(): Unable to receive the requested Storage server IP and PORT.");
        close(nsSocket);
        return -1;
    }
    close(nsSocket);
    return parseIpPort(buffer, IP_address, PORT);
}


int parseIpPort(char *data, char *ip_address,int *ss_port)
{
    if(atoi(data) != ERROR_PATH_DOES_NOT_EXIST) {
        printf("[-] INVALID FILE PATH\n");
        return -1;
    }
    // IP address parsing format "IP:PORT"
    if (sscanf(data, "%[^:]:%d", ip_address, ss_port) != 2){
        printf("[-] Error parsing storage server info: %s\n", data);
        return -1;
    }
    return 0;
}


int connectToStorageServer(const char* IP_address, const int PORT)
{
    /* Connect to the storage server given the IP and PORT */
    int serverSocket;
    if((serverSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        perror("[-] Error connectToStorageServer(): Connecting to Storage server");
        return -1;
    }

    struct sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(PORT);
    serverAddress.sin_addr.s_addr = inet_addr(IP_address);
        
    if (connect(serverSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0){
        perror("[-] Connection to new server failed");
        return -1;
    }
    return serverSocket;
}


int checkResponse(char* response){
    // Print error based on ERROR-CODE (if response!=0)
    if(strcmp(response, VALID_STRING)==0){
        bzero(error_message, ERROR_BUFFER_LENGTH);
        handleErrorCodes(response, error_message);
        printf("[-] %s\n", error_message);
        return -1;
    }
    return 0;
}


int receiveAndCheckResponse(int serverSocket, char* error){
    char response[20];
    if(recv(serverSocket, response, sizeof(response), 0) < 0){
        printf("[-] %s\n", error);
        close(serverSocket);
        return -1;
    }

    int check = checkResponse(response);
    if(check < 0) close(serverSocket);
    return check;
}





int connectAndCheckForFileExistence(char* path, char* operation_num)
{
    int PORT = 0;
    char IP_address[20]; 
    if(fetchStorageServerIP_Port(path, IP_address, &PORT) < 0) {
        return -1;
    }

    int serverSocket = connectToStorageServer(IP_address, PORT);
    if(serverSocket < 0) return serverSocket;

    if(sendPath(serverSocket, operation_num) == -1) return -1;
    if(sendPath(serverSocket, path) == -1) return -1;
    return serverSocket;
}



////////////////////////////// FILE OPERATION /////////////////////////////
void createFile(char* path)
{
    int nsSocket;
    if((nsSocket = sendCommandToNameServer(CREATE_FILE, path)) < 0) 
        return;

    // Sending the file path to the nameserver
    if(sendPath(nsSocket, path) < 0) return;
    close(nsSocket);
}


void deleteFile(char* path){
    int serverSocket = connectAndCheckForFileExistence(path, "6");
    if(serverSocket < 0) return;

    // Checking for confirmation is the server is ready to delete the file
    int check = receiveAndCheckResponse(serverSocket, "Error deleteFile(): Unable to receive the confirmation");
    if(check < 0) return;

    // Checking for confirmation if the file was deleted
    check = receiveAndCheckResponse(serverSocket, "Error deleteFile(): Unable to receive the confirmation");
    if(check < 0) return;
    
    printf("[+] FILE DELETED\n");
    close(serverSocket);
}


void readFile(char* path) // Function to download the specified file
{
    int serverSocket = connectAndCheckForFileExistence(path, "3");
    if(serverSocket < 0) return;

    // Receiving the file
    char filename[MAX_FILE_NAME_LENGTH];
    extractFileName(path, filename);
    downloadFile(filename, serverSocket);
    close(serverSocket);
}


void writeToFile(char* path, char* data) // Function to upload a file to the server
{
    int serverSocket = connectAndCheckForFileExistence(path, "4");
    if(serverSocket < 0) return;

    // Ready to send data
    if(send(serverSocket, "0", 1, 0) < 0){
        perror("[-] Error writeToFile(): Unable to send ready to send message");
        close(serverSocket);
        return;
    }

    // Break data into buffer chunks and send
    size_t data_length = strlen(data);
    size_t sent_bytes = 0;
    size_t chunk_size = BUFFER_LENGTH;

    while (sent_bytes < data_length)
    {
        // Determine the size of the current chunk
        size_t remaining_bytes = data_length - sent_bytes;
        size_t current_chunk_size = (remaining_bytes < chunk_size) ? remaining_bytes : chunk_size;

        // Send the current chunk
        ssize_t bytes_sent = send(serverSocket, data + sent_bytes, current_chunk_size, 0);
        printf("%zd\n", bytes_sent);

        if (bytes_sent == -1) {
            perror("[-] Error writeToFile(): Send failed");
            break;
        }
        sent_bytes += bytes_sent; // Update the total sent bytes
    }

    printf("[+] Uploaded File successfully\n");
    close(serverSocket);
}


void getPermissions(char* path)
{
    int serverSocket = connectAndCheckForFileExistence(path, "5");
    if(serverSocket < 0) return;
    
    // Receiving the file permission from the server
    char buffer[BUFFER_LENGTH];
    if(recv(serverSocket, buffer, BUFFER_LENGTH, 0) < 0){
        perror("[-] Error getPermissions(): receiving the file permission");
        close(serverSocket);
        return;
    }

    // Checking for an error
    int response = atoi(buffer);
    if(response == STORAGE_SERVER_ERROR || response == ERROR_GETTING_FILE_PERMISSIONS){
        checkResponse(buffer);
        return;
    }
    parseMetadata(buffer);
}


// To the formatted requested meta data for a file
void parseMetadata(char *data) 
{
    // Format - filepath : filesize : file_permissions : last_access_time : last_modification_time : creation_time
    char filepath[MAX_PATH_LENGTH], last_access_time[50], last_modified_time[50], creation_time[50];
    long int file_size;
    unsigned int file_permissions;
    sscanf(data, "%[^:]:%ld:%o:%[^:]:%[^:]:%[^:]", filepath, &file_size, &file_permissions, last_access_time, last_modified_time, creation_time);
    
    printf("File Path: %s\n", filepath);
    printf("File Size: %ld bytes\n", file_size);
    printf("File Permissions: %o\n", file_permissions & 0777); // Display in octal format
    printf("Last Access Time: %s\n", last_access_time);
    printf("Last Modification Time: %s\n", last_modified_time);
    printf("Last Status Change Time: %s\n", creation_time);
}



////////////////////////////// DIRECTORY OPERATION ////////////////////////
void createDirectory(char* path){

}

void deletDirectory(char* path){

}



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

        // Creating a file specified a path
        if(op == 1){
            getFilePath(path1);
            createFile(path1);
        }

        // Create a folder
        else if(op == 2){
            getFirectoryPath(path1);
            createDirectory(path1);
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
            fflush(stdin);
            fgets(content, 10000, stdin);
            writeToFile(path1, content);
        }

        // Get file permissions
        else if(op == 5){
            getFilePath(path1);
            getPermissions(path1);  
        }

        // Delete File
        else if(op == 6){
            getFilePath(path1);
            deleteFile(path1);  
        }

        // Delete Directory
        else if(op == 7){
            getFirectoryPath(path1);
            deletDirectory(path1);
        }

        // Copy File
        else if(op == 8){
            getFilePath(path1);
            // copyFile(path1);  
        }

        // Copy Directory
        else if(op == 9){
            getFirectoryPath(path1);
        }

        else printf("[-] Invalid operation\n");
    }
    return 0;
}