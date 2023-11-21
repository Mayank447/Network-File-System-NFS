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


// Functions for directory


// Functions for initialization
int open_a_connection_port(int Port, int num_listener);
void* handleNameServerThread(void* args);
void* handleClientRequest(void*);
int receive_ValidateFilePath(int clientSocket, char* filepath, char* operation_no, int check);


#endif