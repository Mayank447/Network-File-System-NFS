#include "header_files.h"

#ifndef SERVER_H
#define SERVER_H

typedef struct file_struct{
    char filepath[MAX_PATH_LENGTH];   // filepath (wrt root directroy
    int read_write;                   // 0 for read and 1 for write
    int reader_count;
    pthread_mutex_t read_write_lock;
    pthread_mutex_t get_reader_count_lock;  
    struct file_struct* next;  
} File;

char ErrorMsg[1024];


// Functions for files
void createFile(char* path, int clientSocketID);
void deleteFile(char* filename, int clientSocketID);
void getFileMetaData(char* filename, int clientSocketID);
void write_releaseLock(File *file, int clientID);
void read_releaseLock(File *file);


// Functions for directory
void uploadDir_client_to_server(char* directoryname, int clientSocketID);
void sendDir_server_to_client(char* directoryname, int clientSocketID);
void deleteDirectory(const char* path, int clientSocketID);
void getDir(char* directoryname, int clientSocketID);


// Functions for initialization
int open_a_connection_port(int Port, int num_listener);
void handleNameServerThread(void* args);
void* handleClientRequest(void*);
void requestHandler(int requestNo, int clientSocket);
void cleanUpFileStruct();


#endif