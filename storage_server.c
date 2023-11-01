#include "header_files.h"
#include "storage_server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>

#define PATH_BUFFER_SIZE 1024
int socketID; //socketID for the server

/* Signal handler in case Ctrl-Z or Ctrl-D is pressed -> so that the socket gets closed */
void handle_signal(int signum) {
    close(socketID);
    exit(signum);
}

/* Close the socket*/
void closeSocket(){
    close(socketID);
    exit(1);
}

/* Create a file - createFile() */
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

    /*initialize meta data*/
    newFile->size = 0;
    newFile->nextfile = NULL;
    newFile->ownerID = ownerID;
    newFile->reader_count = 0;
    newFile->write_client_id = 0;
    newFile->is_locked = 0;
    newFile->file_type = NULL;
    newFile->file_permissions = 0;
    newFile->last_accessed = 0;
    newFile->last_modified = 0;
    
}

/* Create a Directory - createDir() */
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
    newDir->last_accessed = 0; 
    newDir->last_modified = 0;
    newDir->file_permissions = 0;

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

/* List all the directory contents*/
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

/* Release write lock on a file */
void write_releaseLock(File *file, int clientID) {
    if(file->is_locked == 2 && file->write_client_id == clientID){
        file->is_locked = 0;
    }
}

/* Release read lock on a file */
void read_releaseLock(File *file) {
    if(--file->reader_count == 0 && file->is_locked == 1) {
        file->is_locked = 0;
    }
}

/* Send a file to the client - getFile()*/
void sendFile_server_to_client(char* filename, int clientSocketID)
{
    // Open the file for reading on the server side
    int file = open(filename, O_RDONLY);
    if (file == -1) {
        bzero(ErrorMsg, ERROR_BUFFER_LENGTH);
        sprintf(ErrorMsg, "File not found.");
        if(send(clientSocketID, ErrorMsg, sizeof(ErrorMsg), 0)<0){
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

    // update file access time
    // struct stat file_stat;
    // if (stat(filename, &file_stat) == 0) {
    //     time_t access_time = file_stat.st_atime;

    //     // Convert to a readable format.
    //     struct tm *tm_info = localtime(&access_time);
    //     char time_buffer[26];
    //     strftime(time_buffer, 26, "%Y-%m-%d %H:%M:%S", tm_info);
    //     file->last_accessed = time_buffer;

    // } else {
    //     perror("Error getting file information");
    //     return 1;
    // }

    close(file);
    printf("File %s sent successfully.\n", filename);
}

/* Upload a file from client to server - uploadFile()*/
void uploadFile_client_to_server(char* filename, int clientSocketID)
{
    // Open the file for reading on the server side
    int file = open(filename, O_WRONLY | O_CREAT | O_APPEND, 0666);
    if (file == -1) {
        bzero(ErrorMsg, ERROR_BUFFER_LENGTH);
        sprintf(ErrorMsg, "File not found.");
        if(send(clientSocketID, ErrorMsg, sizeof(ErrorMsg), 0)<0){
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

/* Delete a file - deleteFile() */
void deleteFile(char* filename, int clientSocketID)
{
    if (access(filename, F_OK) != 0) { // File does not exist
        if(send(clientSocketID, "No matching file", 17, 0)<0){
            perror("Unable to send message: No matching files");
        }
        return;
    }
    
    // Check for permission [TODO]
    if (remove(filename) == 0){   
        if(send(clientSocketID, "File deleted successfully", 26, 0)<0){
            perror("Unable to send message: File deleted successfully");
        }
    }
    else {
        if(send(clientSocketID, "Unable to delete the file", 26, 0)<0){
            perror("Unable to send message: Unable to delete the file");
        }
    }
}

/* Delete a folder - deleteDirectory() */
void deleteDirectory(const char* path, int clientSocketID)
{
    DIR *dir;
    struct stat stat_path, stat_entry;
    struct dirent *entry;

    // stat for the path
    stat(path, &stat_path);

    // if path does not exists or is not dir - exit with status -1
    if (S_ISDIR(stat_path.st_mode) == 0) {
        bzero(ErrorMsg, ERROR_BUFFER_LENGTH);
        sprintf(ErrorMsg, "Is not directory: %s\n", path);
        if(send(clientSocketID, ErrorMsg, sizeof(ErrorMsg), 0) < 0){
            perror("Unable to send: Is not directory");
        }
        return;
    }

    // if not possible to read the directory for this user
    if ((dir = opendir(path)) == NULL) {
        bzero(ErrorMsg, ERROR_BUFFER_LENGTH);
        sprintf(ErrorMsg, "Can`t open directory: %s\n", path);
        if(send(clientSocketID, ErrorMsg, sizeof(ErrorMsg), 0) < 0){
            perror("Unable to send: Can`t open directory");
        }
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;  // Skip current and parent directories
        }

        char entryPath[MAX_PATH_LENGTH];
        snprintf(entryPath, sizeof(entryPath), "%s/%s", path, entry->d_name);

        if (entry->d_type == 4) { // Recursive call for subdirectories
            deleteDirectory(entryPath, clientSocketID);
        } 

        else {
            deleteFile(entryPath, clientSocketID); // Delete files

            if (remove(entryPath) != 0) {
                bzero(ErrorMsg, ERROR_BUFFER_LENGTH);
                sprintf(ErrorMsg, "Error deleting file: %s", entryPath);
                if(send(clientSocketID, ErrorMsg, sizeof(ErrorMsg), 0) < 0) {
                    printf("%s ", ErrorMsg);
                    perror("Error sending message:");
                }
            }
        }
    }

    closedir(dir);

    // Remove the empty directory after deleting its contents
    if (rmdir(path) != 0) {
        bzero(ErrorMsg, ERROR_BUFFER_LENGTH);
        sprintf(ErrorMsg, "Error: Deleting the repository %s", path);
        if(send(clientSocketID, ErrorMsg, sizeof(ErrorMsg), 0) < 0) {
            printf("%s ", ErrorMsg);
            perror("Error sending message:");
        } 
    }
}

// Copy files between 2 servers - copyFilesender()*/                -------------------------- implement later
// int copyFile_sender(const char *sourcePath, struct sockaddr_in server_address) {
//     int serverSocket;
//     if(connect(serverSocket, (struct sockaddr*)&server_address, sizeof(server_address)) == -1){
//         perror("Error connecting to server");
//         return 1;
//     }
//     sendFile_server_to_client(sourcePath, serverSocket);
//     return 0;
// }

// // Copy files between 2 servers - copyFileReceiver()*/                -------------------------- implement later
// int copyFile_receiver(const char *destinationPath, struct sockaddr_in server_address) {
//     int serverSocket;
//     if(connect(serverSocket, (struct sockaddr*)&server_address, sizeof(server_address)) == -1){
//         perror("Error connecting to server");
//         return 1;
//     }
//     uploadFile_client_to_server(destinationPath, serverSocket);
//     return 0;
// }

/* Function to rename file */
int renameFile(const char *oldFileName, const char *newFileName, int clientSocketID) {
    // Attempt to rename the file
    if (rename(oldFileName, newFileName) == 0) {
        // Successfully renamed the file, send a success response to the client
        char response[1024];
        snprintf(response, sizeof(response), "RENAME_SUCCESS %s %s", oldFileName, newFileName);
        if (send(clientSocketID, response, strlen(response), 0) < 0) {
            perror("Error sending rename success response to the client");
        }
        return 0; // Successfully renamed the file
    } else {
        // Failed to rename the file, send an error response to the client
        char response[1024];
        snprintf(response, sizeof(response), "RENAME_ERROR %s %s", oldFileName, newFileName);
        if (send(clientSocketID, response, strlen(response), 0) < 0) {
            perror("Error sending rename error response to the client");
        }
        perror("Error renaming the file");
        return -1; // Failed to rename the file
    }
}

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

int clientSocket[MAX_CLIENT_CONNECTIONS];
char filename[50]="paths_SS.txt";
// Function to collect accessible paths from the user and store them in a file
void collectAccessiblePaths() {
    char path[PATH_BUFFER_SIZE];
    FILE *file = fopen(filename, "a"); // Open the file in append mode
    if (file == NULL) {
        perror("Error opening paths_SS.txt");
        return;
    }
    while (1) {
        printf("Enter an accessible path (or 'exit' to stop): ");
        fgets(path, sizeof(path), stdin);
        if (strcmp(path, "exit\n") == 0) {
            break;
        }
        fprintf(file, "%s", path); // Write the path to the file
    }

    fclose(file);
}
int namingServerNumber;
// Function to send vital information to the Naming Server and receive a number
int sendInfoToNamingServer(const char* nsIP, int nsPort, int clientPort) {
    struct sockaddr_in nsAddress;
    int nsSocket;

    // Create a socket for communication with the Naming Server
    if ((nsSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Error: opening socket for Naming Server");
        return -1;
    }
    memset(&nsAddress, 0, sizeof(nsAddress));
    nsAddress.sin_family = AF_INET;
    nsAddress.sin_port = htons(9090);
    nsAddress.sin_addr.s_addr = inet_addr(nsIP);
    // Connect to the Naming Server
    if (connect(nsSocket, (struct sockaddr*)&nsAddress, sizeof(nsAddress)) < 0) {
        perror("Error: connecting to Naming Server");
        //close(nsSocket);
        return -1;
    }
    // Prepare the information to send
    char infoBuffer1[PATH_BUFFER_SIZE];
    snprintf(infoBuffer1, sizeof(infoBuffer1), "SENDING STORAGE SERVER INFORMATION");
    
    // Send the information to the Naming Server
    if (send(nsSocket, infoBuffer1, strlen(infoBuffer1), 0) < 0) {
        perror("Error: sending information to Naming Server");
        //close(nsSocket);
        return -1;
    }
    char infoBuffer[PATH_BUFFER_SIZE];
    snprintf(infoBuffer, sizeof(infoBuffer), "%s:%d:%d", nsIP, nsPort, clientPort);

    // Send the information to the Naming Server
    if (send(nsSocket, infoBuffer, strlen(infoBuffer), 0) < 0) {
        perror("Error: sending information to Naming Server");
        //close(nsSocket);
        return -1;
    }

    // Open the file for reading
    FILE* pathFile = fopen(filename, "r");
    if (pathFile == NULL) {
        perror("Error opening path file");
        //close(nsSocket);
        return -1;
    }
    // Read and send each path
    char path[PATH_BUFFER_SIZE];
    while (fgets(path, sizeof(path), pathFile) != NULL) {
        if (send(nsSocket, path, strlen(path), 0) < 0) {
            perror("Error: sending path to Naming Server");
            fclose(pathFile);
            //close(nsSocket);
            return -1;
        }
    }

    // Send a "completed" message
    const char* completedMessage = "COMPLETED";
    if (send(nsSocket, completedMessage, strlen(completedMessage), 0) < 0) {
        perror("Error: sending completed message to Naming Server");
        fclose(pathFile);
        //close(nsSocket);
        return -1;
    }

    // Close the file and the socket
    fclose(pathFile);
    //close(nsSocket);

    // Receive a number from the Naming Server
    char responseBuffer[PATH_BUFFER_SIZE];
    if (recv(nsSocket, responseBuffer, sizeof(responseBuffer), 0) < 0) {
        perror("Error: receiving number from Naming Server");
        return -1;
    }
    namingServerNumber = atoi(responseBuffer);

    return namingServerNumber;
}
int main(int argc, char* argv[]) {
    // Signal handler for Ctrl+C and Ctrl+Z
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);
    // Ask the user for the Naming Server's IP and port
    char nsIP[16]; // Assuming IPv4
    int nsPort;
    printf("Enter the IP address of the Naming Server: ");
    scanf("%s", nsIP);
    printf("Enter the port to talk with the Naming Server: ");
    scanf("%d", &nsPort);
    // Ask the user for the client communication port
    int clientPort;
    printf("Enter the port to talk with the Client: ");
    scanf("%d", &clientPort);
    // Creating a socket
    // Collect accessible paths from the user and store in a file
    collectAccessiblePaths();
    // Send vital information to the Naming Server and receive a number
    int ServerNumber = sendInfoToNamingServer(nsIP, nsPort, clientPort);
    if (ServerNumber == -1) {
        printf("Failed to send information to the Naming Server.\n");
    } else {
        snprintf(filename, sizeof(filename), "path_SS%d.txt", ServerNumber);
        rename("path_SS.txt",filename);
        printf("Received Naming Server number: %d\n", ServerNumber);
    }

    // Close the socket
    // if (close(socketID) < 0) {
    //     perror("Error: closing socket\n");
    //     exit(1);
    // }
    return 0;
}
