#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "storage_server.h"

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
    newFile->write_client_id = NULL;
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

/* Function to send a file getFile()*/
void getFile(char* filename, int clientSocketID){

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

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddress.sin_port = htons(PORT);

    // Binding the socket to the server address
    if(bind(socketID, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0){
        perror("Error: binding socket\n");
        exit(1);
    }
    printf("Storage Server has successfully started on port %d", PORT);

    // Sending the vital info to Name Server: IP Address, PORT FOR NS communication, PORT for SS communication, all accessible paths


    // Closing the socket
    if(close(socketID) < 0){
        perror("Error: closing socket\n");
        exit(1);
    }
    return 0;
}
