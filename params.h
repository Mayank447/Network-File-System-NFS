#ifndef PARAMS_H
#define PARAMS_H


// Operations
#define CREATE_FILE "1"
#define CREATE_DIRECTORY "2"
#define READ_FILE "3"
#define WRITE_FILE "4"
#define GET_FILE_PERMISSIONS "5"
#define DELETE_FILE "6"
#define DELETE_DIRECTORY "7"
#define COPY_FILES "8"
#define COPY_DIRECTORY "9"


#define CLIENT_PORT 8080 // Port no. for communication with the client
#define STORAGE_SERVER_PORT 9090 // Port no. for communication with the storage server

#define NO_CLIENTS_TO_LISTEN_TO 10 // Maximum no. of clients the name server can handle
#define NO_SERVER_TO_LISTEN_TO 10 // Maximum no. of storage servers can initialize

#define MAX_SERVERS 100 // Maximum no. of thread which can be managed for server
#define MAX_CLIENTS 100 // Maximum no. of thread which can be managed for client

#define BUFFER_LENGTH 1000 // Max length of string received from Storage Server for initialization
#define ERROR_BUFFER_LENGTH 1024
#define MAX_PATH_LENGTH 1000
#define MAX_FILE_NAME_LENGTH 256
#define MAX_NO_ACCESSIBLE_PATHS 300
#define MAX_LENGTH_OF_ACCESSIBLE_PATH 300

#define MAX_CLIENT_CONNECTIONS 5
#define RECEIVE_BUFFER_LENGTH 1024
#define SEND_BUFFER_LENGTH 1024
#define ERROR_BUFFER_LENGTH 1024
#define MAX_PATH_LENGTH 1000
#define PATH_BUFFER_SIZE 1024

#define RECEIVE_THREAD_RUNNING_TIME 5

// Ensuring the Storage Server is running by sending a pulse
#define PERIODIC_HEART_BEAT 2
#define NOT_RECEIVED_COUNT 3

#endif