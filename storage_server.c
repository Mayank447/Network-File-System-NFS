#include "storage_server.h"

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

/* Function to create a file - createFile() */
void createFile(Directory *parent, const char *filename, int ownerID)
{
    File* newFile = (File*)malloc(sizeof(File));
    if(newFile == NULL){
        printf("createFile: malloc failed\n");
        return;
    }

    strcpy(newFile->name, filename);
    strcpy(newFile->path, parent->path);
    strcat(newFile->path, filename);

    newFile->size = 0;
    newFile->nextfile = NULL;
    newFile->ownerID = ownerID;
    newFile->reader_count = 0;
    newFile->write_client_id = 0;
    newFile->is_locked = 0;
}


/* Function to create a Directory - createDir() */
void createDir(Directory *parent, const char *dirname)
{
    Directory* newDir = (Directory*)malloc(sizeof(Directory));
    if(newDir == NULL){
        printf("createDir: malloc failed\n");
        return;
    }

    strcpy(newDir->name, dirname);
    newDir->file_count = 0;
    newDir->subdir_count = 0;
    newDir->next_subDir = NULL;
    newDir->file_head = newDir->file_tail = NULL;
    newDir->subdir_head = newDir->subdir_tail = NULL;

    // Inserting the new node at the end of the parent linked list
    if(parent == NULL){
        strcpy(newDir->path, "/");
    }
    else{
        strcpy(newDir->path, parent->path);
        strcat(newDir->path, dirname);
        strcat(newDir->path, "/");

        if(parent->subdir_count == 0){
            parent->subdir_head = parent->subdir_tail = newDir;
            parent->subdir_count++;
            return;
        }
        parent->subdir_tail->next_subDir = newDir;
        parent->subdir_count++;
    }   
}


/* Function to list all the directory contents*/
void listDirectoryContents(Directory* parent)
{
    // Printing all the subdirectories
    for(Directory* dir = parent->subdir_head; dir!=NULL; dir = dir->next_subDir){
        printf("%s\n", dir->name);
    }

    // Printing all the files
    for(File* file = parent->file_head; file!=NULL; file = file->nextfile){
        printf("%s \t (%d bytes \t ownerID: %d)\n", file->name, file->size, file->ownerID);
    }
}

/* Lock a File */
int lockFile(File *file, int lock_type) 
{
    if (file->is_locked == 0) {
        file->is_locked = lock_type;
        return 0;
    }

    else if(lock_type==1 && file->is_locked==1){
        file->reader_count++;
        return 0;
    }
    return 1;
}

/* Release lock on a file */
void write_releaseLock(File *file, int clientID) {
    if(file->is_locked == 2 && file->write_client_id == clientID){
        file->is_locked = 0;
    }
}

void read_releaseLock(File *file) {
    if(--file->reader_count == 0 && file->is_locked == 1) {
        file->is_locked = 0;
    }
}


/* Function to send a file to the client - getFile()*/
void sendFile_server_to_client(char* filename, int clientSocketID)
{
    // Open the file for reading on the server side
    int file = open(filename, O_RDONLY);
    if (file == -1) {
        char errorMsg[] = "File not found.";
        if (send(clientSocketID, errorMsg, sizeof(errorMsg), 0) < 0) {
            perror("Failed to send file not found error");
        }
        return;
    }

    char buffer[SEND_BUFFER_LENGTH];
    ssize_t bytesRead;

    while ((bytesRead = read(file, buffer, sizeof(buffer))) > 0) {
        if (send(clientSocketID, buffer, bytesRead, 0) < 0) {
            perror("Error sending file");
            close(file);
            return;
        }
    }

    close(file);
    printf("File %s sent successfully.\n", filename);
}


/* Function to upload a file - */
void uploadFile_client_to_server(char* filename, int clientSocketID)
{
    // Open the file for reading on the server side
    int file = open(filename, O_WRONLY | O_CREAT | O_APPEND, 0666);
    if (file == -1) {
        char errorMsg[] = "File not found.";
        if(send(clientSocketID, errorMsg, sizeof(errorMsg), 0)<0){
            perror("Failed to send file not found error");
        }
        return;
    }

    char buffer[RECEIVE_BUFFER_LENGTH];
    int bytesReceived;

    // Receive and write the file content
    while ((bytesReceived = recv(clientSocketID, buffer, sizeof(buffer), 0)) > 0) {
        if(write(file, buffer, bytesReceived) < 0){
            perror("Error writing to file");
            close(file);
            return;
        }
    }

    if (bytesReceived < 0) {
        perror("Error receiving file data");
    }

    close(file);
    printf("File %s received successfully.\n", filename);
}

// /*Function to copy files copyFile()*/                -------------------------- implement later
// int copyFile(const char *sourcePath, const char *destinationPath) {
//     FILE *sourceFile, *destinationFile;
//     char ch;

//     sourceFile = fopen(sourcePath, "rb");
//     if (sourceFile == NULL) {
//         perror("Unable to open source file");
//         return 1;
//     }

//     destinationFile = fopen(destinationPath, "wb");
//     if (destinationFile == NULL) {
//         perror("Unable to create or open destination file");
//         fclose(sourceFile);
//         return 1;
//     }

//     while ((ch = fgetc(sourceFile)) != EOF) {
//         fputc(ch, destinationFile);
//     }

//     fclose(sourceFile);
//     fclose(destinationFile);

//     return 0;
// }

// /*Function to copy directories copyDirectory()*/     -------------------------- implement later
// void copyDirectory(const char *sourceDir, const char *destinationDir) {
//     struct dirent *entry;
//     struct stat statInfo;
//     char sourcePath[MAX_PATH_LENGTH], destinationPath[MAX_PATH_LENGTH];

//     // Open source directory
//     DIR *dir = opendir(sourceDir);

//     if (dir == NULL) {
//         perror("Unable to open source directory");
//         return;
//     }

//     // Create destination directory
//     mkdir(destinationDir, 0777);

//     while ((entry = readdir(dir))) {
//         if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
//             sprintf(sourcePath, "%s/%s", sourceDir, entry->d_name);
//             sprintf(destinationPath, "%s/%s", destinationDir, entry->d_name);

//             if (stat(sourcePath, &statInfo) == 0) {
//                 if (S_ISDIR(statInfo.st_mode)) {
//                     // Recursively copy subdirectories
//                     copyDirectory(sourcePath, destinationPath);
//                 } else {
//                     // Copy files
//                     copyFile(sourcePath, destinationPath);
//                 }
//             }
//         }
//     }

//     closedir(dir);
// }

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
    struct sockaddr_in serverAddress, nameServerAddress;
    int addrlen = sizeof(serverAddress);
    memset(&serverAddress, 0, sizeof(serverAddress));
    memset(&nameServerAddress, 0, sizeof(nameServerAddress));

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddress.sin_port = htons(PORT);

    // Binding the socket to the server address
    if(bind(socketID, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0){
        perror("Error: binding socket\n");
        exit(1);
    }
    printf("Storage Server has successfully started on port %d\n", PORT);

    // Sending the vital info to Name Server: IP Address, PORT FOR NS communication, PORT for SS communication, all accessible paths

    // Listening for clients
    if(listen(socketID, MAX_CLIENT_CONNECTIONS)<0){
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }


    // Initializing empty sockets and clientAddresses for clients
    int clientSocket[MAX_CLIENT_CONNECTIONS];
    struct sockaddr_in clientAddress[MAX_CLIENT_CONNECTIONS];
    int client_socket_count = 0;
    
    for(int j=0; j<MAX_CLIENT_CONNECTIONS; j++)
        memset(&clientAddress[j], 0, sizeof(clientAddress[j]));


    // Accepting connections from clients
    while(1){
        if ((clientSocket[client_socket_count] = accept(socketID, (struct sockaddr*)&clientAddress[client_socket_count], (socklen_t*)&addrlen)) < 0) {
            perror("Accept failed");
            exit(EXIT_FAILURE);
        }
        else{
            client_socket_count++;
            printf("New connection..\n");
        }

    }

    // Closing the socket
    if(close(socketID) < 0){
        perror("Error: closing socket\n");
        exit(1);
    }
    return 0;
}
