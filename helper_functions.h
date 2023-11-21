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

// print the error based on the valid bit to message string
void handleErrorCodes(char* valid, char* message);
void handleErrorCodes(char* valid, char* message);
void printError(char* response);

int nonBlockingRecv(int serverSocket, char* buffer);
int nonBlockingRecvPeriodic(int serverSocket, char* buffer);

int checkOperationNumber(char* buffer);
int receiveOperationNumber(int socket);
int receivePath(int socket, char* buffer);
int sendData(int socket, char* response);
int sendConfirmation(int socket);
int receiveConfirmation(int serverSocket);
int sendPath(int socket, char* buffer);
int open_a_connection_port(int Port, int num_listener);
int connectToServer(const char* IP_address, const int PORT);
int sendDataAndReceiveConfirmation(int socket, char* data);

#endif