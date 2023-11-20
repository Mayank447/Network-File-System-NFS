#ifndef HELPER_FUNCTIONS
#define HELPER_FUNCTIONS

#define THREAD_RUNNING  0
#define THREAD_FINISHED 1
struct ReceiveThreadArgs{
    int serverSocket;
    char* buffer;
    int threadStatus;
    int threadResult;
};


// Remove leading whitespaces form a string
void trim(char *str); 

// Extract filename from path
void extractFileName(char *path, char *filename); 

// Download a File - Receive data from a Socket and write to a File
void downloadFile(char* filename, int socket); 

// Upload a File - Read a File and send data to a Socket
void uploadFile(char* filename, int socket); 

// print the error based on the valid bit to message string
void handleErrorCodes(char* valid, char* message);
void handleErrorCodes(char* valid, char* message);
int createRecvThread(int serverSocket, char* buffer);
int createRecvThreadPeriodic(int serverSocket, char* buffer);

int checkOperationNumber(char* buffer);
int sendReponse(int socket, char* response);
int sendConfirmation(int socket);
int receiveConfirmation(int serverSocket, char* buffer);

#endif