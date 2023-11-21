#include "header_files.h"
#include "client.h"
#include "file.h"

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
void getDirectoryPath(char* path){
    printf("Enter the directory path: ");
    scanf("%s", path);
}




///////////////////////// FETCHING AND CONNECTING TO STORAGE SERVER //////////////////

// Function to connect to the storage server and send the operation number, path and receive confirmation for the same
int sendOperation_PathToNameServer(char* operation_num, char* path)
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
    if(sendDataAndReceiveConfirmation(nsSocket, operation_num)){
        perror("[-] Error sendCommandToNameServer(): Unable to send operation number to the Nameserver");
        close(nsSocket);
        return -1;
    }

    // Sending the file path to the nameserver and receiving confirmation
    if(sendDataAndReceiveConfirmation(nsSocket, path) < 0) {
        printf("[-] Error sending filepath to the nameserver\n");
        close(nsSocket);
        return -1;
    }

    if(sendConfirmation(nsSocket)){
        printf("[-] Error sending confirmation on received confirmation.\n");
        close(nsSocket);
        return -1;
    }

    return nsSocket;
}


// Wrapper Function to perform operations done by Name Server
int performNSOperation(char* op, char* path)
{
    int nsSocket;
    if((nsSocket = sendOperation_PathToNameServer(op, path)) < 0) {
        printf("[-] Error sending/receiving Operation No./Path to the Nameserver\n");
        close(nsSocket);
        return -1;
    }

    if(receiveConfirmation(nsSocket)){
        close(nsSocket);
        return -1;
    }

    close(nsSocket);
    return 0;
}


// Function to fetch the Storage server IP and PORT
int getStorageServerIP_Port(int nsSocket, char* IP_address, int* PORT)
{   
    char buffer[BUFFER_LENGTH];
    if(nonBlockingRecv(nsSocket, buffer)){
        printf("[-] Error receiving Storage Server IP and PORT\n");
        close(nsSocket);
        return -1;
    }

    close(nsSocket);
    return parseIpPort(buffer, IP_address, PORT);
}


// Parsing the IP address and PORT for the storage server
int parseIpPort(char *data, char *ip_address,int *ss_port)
{
    // IP address parsing format "IP:PORT"
    if (sscanf(data, "%[^:]:%d", ip_address, ss_port) != 2){
        printf("[-] Error parsing storage server info: %s\n", data);
        printError(data);
        return -1;
    }
    return 0;
}




////////////////////////////// FILE OPERATION /////////////////////////////

// Function to download the specified file
int readFile(char* path) 
{
    int nsSocket, PORT;
    char IP_address[20], filename[MAX_FILE_NAME_LENGTH];;

    if((nsSocket = sendOperation_PathToNameServer(READ_FILE, path)) == -1){
        return -1;
    }

    if(getStorageServerIP_Port(nsSocket, IP_address, &PORT)){
        printf("[-] Error fetching Storage Server IP Address or PORT\n");
        return -1;
    }

    int serverSocket = connectToServer(IP_address, PORT);
    if(serverSocket < 0) return -1;
    
    if(sendDataAndReceiveConfirmation(serverSocket, READ_FILE)){
        printf("[-] Error sending or receiving confirmation for Operation Number\n");
        close(serverSocket);
        return -1;
    }

    if(sendDataAndReceiveConfirmation(serverSocket, path)){
        printf("[-] Error sending or receiving confirmation for Path\n");
        close(serverSocket);
        return -1;
    }

    // Receiving the file
    extractFileName(path, filename);
    DownloadFile(serverSocket, path);
    close(serverSocket);
    return 0;
}


// Function to write to a file on the server
int writeToFile(char* path, char* data) 
{
    int nsSocket, PORT;
    char IP_address[20];

    if((nsSocket = sendOperation_PathToNameServer(WRITE_FILE, path)) == -1){
        return -1;
    }

    if(getStorageServerIP_Port(nsSocket, IP_address, &PORT)){
        printf("[-] Error fetching Storage Server IP Address or PORT\n");
        return -1;
    }

    int serverSocket = connectToServer(IP_address, PORT);
    if(sendDataAndReceiveConfirmation(serverSocket, WRITE_FILE)){
        printf("[-] Error sending or receiving confirmation for Operation Number\n");
        close(serverSocket);
        return -1;
    }

    if(sendDataAndReceiveConfirmation(serverSocket, path)){
        printf("[-] Error sending or receiving confirmation for Path\n");
        close(serverSocket);
        return -1;
    }

    // Ready to send data
    if(send(serverSocket, "0", 1, 0) < 0){
        perror("[-] Error writeToFile(): Unable to send ready to send message");
        close(serverSocket);
        return -1;
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
            close(serverSocket);
            return -1;
        }
        sent_bytes += bytes_sent; // Update the total sent bytes
    }

    printf("[+] Uploaded File successfully\n");
    close(serverSocket);
    return 0;
}


// Function to get the file permissions
int getPermissions(char* path)
{
    int nsSocket, PORT;
    char IP_address[20];

    if((nsSocket = sendOperation_PathToNameServer(GET_FILE_PERMISSIONS, path)) == -1){
        return -1;
    }

    if(getStorageServerIP_Port(nsSocket, IP_address, &PORT)){
        printf("[-] Error fetching Storage Server IP Address or PORT\n");
        return -1;
    }

    int serverSocket = connectToServer(IP_address, PORT);
    if(serverSocket < 0) return -1;
    
    if(sendDataAndReceiveConfirmation(serverSocket, GET_FILE_PERMISSIONS)){
        printf("[-] Error sending or receiving confirmation for Operation Number\n");
        close(serverSocket);
        return -1;
    }

    if(sendDataAndReceiveConfirmation(serverSocket, path)){
        printf("[-] Error sending or receiving confirmation for Path\n");
        close(serverSocket);
        return -1;
    }
    
    // Receiving the file permission from the server
    char buffer[BUFFER_LENGTH];
    if(nonBlockingRecv(serverSocket, buffer)){
        close(serverSocket);
        return -1;
    }

    // Checking for an error
    if(atoi(buffer) != 0){
        printError(buffer);
        return -1;
    }
    parseMetadata(buffer);
    return 0;
}


// To the format requested meta data for a file
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
            if(performNSOperation(CREATE_FILE, path1) != -1){
                printf("[+] File created successfully\n");
            }
        }

        // Create a folder
        else if(op == 2){
            getDirectoryPath(path1);
            if(performNSOperation(CREATE_DIRECTORY, path1) != -1){
                printf("[+] Directory created successfully\n");
            }
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
            if(performNSOperation(DELETE_FILE, path1) != -1){
                printf("[+] File deleted successfully\n");
            }  
        }

        // Delete Directory
        else if(op == 7){
            getDirectoryPath(path1);
            if(performNSOperation(DELETE_DIRECTORY, path1) != -1){
                printf("[+] Directory deleted successfully\n");
            }  
        }

        // Copy File
        else if(op == 8){
            getFilePath(path1);
            if(performNSOperation(COPY_FILES, path1) != -1){
                printf("[+] Directory deleted successfully\n");
            }  
        }

        // Copy Directory
        else if(op == 9){
            getDirectoryPath(path1);
            if(performNSOperation(COPY_DIRECTORY, path1) != -1){
                printf("[+] Directory deleted successfully\n");
            } 
        }

        else printf("[-] Invalid operation\n");
    }
    return 0;
}