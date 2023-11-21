#include "header_files.h"
#include "helper_functions.h"

char error_message[ERROR_BUFFER_LENGTH];

// Function to handle the error codes based on Nameserver/storage server response
void handleErrorCodes(char* response, char* message)
{
    int valid = atoi(response);

    if(strcmp(response, "0") == 0) 
        strcpy(message, "VALID");

    else if(valid == NAME_SERVER_ERROR)
        strcpy(message, "NAMESERVER ERROR");

    else if(valid == ERROR_PATH_DOES_NOT_EXIST) 
        strcpy(message, "PATH DOES NOT EXIST");

    else if(valid == STORAGE_SERVER_DOWN)
        strcpy(message, "STORAGE SERVER IS DOWN");

    else if(valid == ERROR_A_CLIENT_ALREADY_READING_THE_FILE)
        strcpy(message, "A CLIENT IS ALREADY READING THE FILE");

    else if(valid == ERROR_A_CLIENT_ALREADY_WRITING_TO_FILE)
        strcpy(message, "A CLIENT IS ALREADY WRITING TO THE FILE");

    else if(valid == ERROR_DIRECTORY_DOES_NOT_EXIST)
        strcpy(message, "FILE DOES NOT EXIST");

    else if(valid == ERROR_DIRECTORY_ALREADY_EXISTS)
        strcpy(message, "FILE ALREADY EXISTS");

    else if(valid == ERROR_DIRECTORY_DOES_NOT_EXIST)
        strcpy(message, "DIRECTORY DOES NOT EXIST");

    else if(valid == ERROR_DIRECTORY_ALREADY_EXISTS)
        strcpy(message, "DIRECTORY ALREADY EXIST");

    else if(valid == ERROR_INVALID_REQUEST_NUMBER)
        strcpy(message, "INVALID OPERATION NUMBER");

    else if(valid == STORAGE_SERVER_ERROR)
        strcpy(message, "STORAGE SERVER ERROR");

    else if(valid == ERROR_CREATING_FILE)
        strcpy(message, "ERROR CREATING FILE");

    else if(valid == ERROR_OPENING_FILE)
        strcpy(message, "SERVER ERROR: OPENING FILE");

    else if(valid == ERROR_UNABLE_TO_DELETE_FILE)
        strcpy(message, "SERVER ERROR: UNABLE TO DELETE FILE");

    else if(valid == ERROR_GETTING_FILE_PERMISSIONS)
        strcpy(message, "SERVER ERROR: ERROR GETTING FILE PERMISSIONS");
}

// Function to print error given the string contaning the error code 
void printError(char* response){
    char error_message[ERROR_BUFFER_LENGTH];
    handleErrorCodes(response, error_message);
    printf("[-] %s\n", error_message);
}

// Remove leading whitespaces
void trim(char *str) {
    int i=0, j=0, len = strlen(str);
    while(isspace((unsigned char)str[i]) && i<len) i++;
    while(i<len && !isspace((unsigned char)str[i])) str[j++] = str[i++];
    str[j] = '\0';
}

// Extract filename from path
void extractFileName(char *path, char *filename) {
    const char *lastSlash = strrchr(path, '/');

    if (lastSlash != NULL) {
        strcpy(filename, lastSlash + 1);
    } else {
        strcpy(filename, path);
    }
}


// Thread Function to receive data from the socket
void* receiveInfo(void* thread_args)
{
    struct ReceiveThreadArgs* args = (struct ReceiveThreadArgs*)thread_args;
    
    // Receiving the confirmation for storage server's side
    if(recv(args->serverSocket, args->buffer, BUFFER_LENGTH, 0) < 0){
        perror("[-] Error createFile(): Unable to receive info");
        args->threadResult = -1;
        args->threadStatus = THREAD_FINISHED;
        return NULL;
    }
    pthread_testcancel();
    args->threadResult = 0;
    args->threadStatus = THREAD_FINISHED;
    return NULL;
}


// Thread creating function for receiveInfo()
int nonBlockingRecv(int serverSocket, char* buffer)
{
    struct ReceiveThreadArgs args;
    args.buffer = buffer;
    args.serverSocket = serverSocket;
    args.threadStatus = THREAD_RUNNING;
    args.threadResult = -1;

    pthread_t receiveThread;
    bzero(buffer, BUFFER_LENGTH);
    pthread_create(&receiveThread, NULL, (void*)receiveInfo, (void*)&args);
    
    // Waiting for a specified time, if the thread does not finish exit
    clock_t start_time = clock();
    while((args.threadStatus != THREAD_FINISHED) && ((double)(clock() - start_time))/ CLOCKS_PER_SEC  <  RECEIVE_THREAD_RUNNING_TIME);
    
    if(args.threadStatus == THREAD_FINISHED) {
        pthread_join(receiveThread, NULL);
        return args.threadResult;
    }
    
    pthread_cancel(receiveThread);
    printf("[-] Failed to receive sent info\n");
    return -1;
}


// Thread creating function for receiveInfo() every 5s
int nonBlockingRecvPeriodic(int serverSocket, char* buffer)
{
    struct ReceiveThreadArgs args;
    args.buffer = buffer;
    args.serverSocket = serverSocket;
    args.threadStatus = THREAD_RUNNING;
    args.threadResult = -1;

    pthread_t receiveThread;
    bzero(buffer, BUFFER_LENGTH);
    pthread_create(&receiveThread, NULL, (void*)receiveInfo, (void*)&args);
    
    // Waiting for a specified time, if the thread does not finish exit
    clock_t start_time = clock();
    while(((double)(clock() - start_time))/ CLOCKS_PER_SEC  <  PERIODIC_HEART_BEAT);
    
    if(args.threadStatus == THREAD_FINISHED) {
        pthread_join(receiveThread, NULL);
        return args.threadResult;
    }
    
    pthread_cancel(receiveThread);
    printf("[-] Failed to receive sent info\n");
    return -1;
}


// Thread to send response
int sendData(int socket, char* response){
    if(send(socket, response, strlen(response), 0) < 0){
        perror("[-] sendConfirmation(): Error sending confirmation");
        return -1;
    }
    return 0;
}

// Thread to send confirmation of valid
int sendConfirmation(int socket){
    if(send(socket, VALID_STRING, strlen(VALID_STRING), 0) < 0){
        return -1;
    }
    return 0;
}

// Wrapper around nonBlockingRecv to check for confirmation
int receiveConfirmation(int serverSocket)
{
    char buffer[BUFFER_LENGTH];
    if(nonBlockingRecv(serverSocket, buffer)) 
        return -1;
    
    if(strcmp(buffer, VALID_STRING) != 0) {
        printError(buffer);
        return -1;
    }
    return 0;
}

// Checking the operation number
int checkOperationNumber(char* buffer)
{
    int num = atoi(buffer);
    if(num <= 0 || num > 9) return -1;
    bzero(buffer, BUFFER_LENGTH);
    return num;
}


int receiveOperationNumber(int socket)
{
    char buffer[BUFFER_LENGTH], response[100];
    if(nonBlockingRecv(socket, buffer)){
        printf("[-] Unable to receive the operation number from the nameserver\n");
        return -1;
    }

    int op = checkOperationNumber(buffer);
    if(op == -1){
        sprintf(response, "%d", ERROR_INVALID_REQUEST_NUMBER);
        printf("[-] Invalid Operation number\n");
    }
    else {
        strcpy(response, VALID_STRING);
    }


    //Sending the confirmation for the received operation number
    if(sendData(socket, response)){
        printf("[-] Unable to send the response\n");
        return -1;
    }

    if(op == -1) return -1;
    return op;
}

int receivePath(int socket, char* buffer)
{
    // Receiving the path
    if(nonBlockingRecv(socket, buffer) != 0) 
        return -1;

    // Additional checks on path if any (TODO)


    // Sending confirmation the path
    if(sendConfirmation(socket) != 0){
        printf("[-] Error sending the confirmation for the path\n");
        return -1;
    }

    return 0;
}


// Download a File - Receive data from a Socket and write to a File
void downloadFile(char* filename, int socket)
{
    FILE* file = fopen(filename, "w");
    if(!file){
        printf("Unable to open the FILE %s for writing\n", filename);
        return;
    }

    // Waiting for FILE Opened message
    char buffer[BUFFER_LENGTH];
    if(nonBlockingRecv(socket, buffer)){
        perror("[-] Error downloadFile() - Unable to receive the file open confirmation");
        fclose(file);
        return;
    }

    if(strcmp(buffer, VALID_STRING) != 0){
        printError(buffer);
        fclose(file);
        return;
    }

    // Receiving the FILE DATA
    while(1)
    {
        if(nonBlockingRecv(socket, buffer)){
            perror("[-] Error downloadFile(): Unable to receive the file open confirmation");
            fclose(file);
            return;
        }

        if(strcmp(buffer, "COMPLETE") == 0) break;

        if(fprintf(file, "%s", buffer) < 0){
            printf("Error downloadFile(): Unable to write to the file");
            fclose(file);
            return;
        }
    }
    fclose(file);
    printf("File %s downloaded successfully.\n", filename);
}


void uploadFile(char *filename, int clientSocket)
{
    // Open the file for reading on the server side
    FILE *file = fopen(filename, "r");
    bzero(error_message, ERROR_BUFFER_LENGTH);

    if (!file) sprintf(error_message, "%d", ERROR_OPENING_FILE);
    else sprintf(error_message, VALID_STRING);

    if(sendData(clientSocket, error_message)){
        perror("[-] Error UploadingFile(): Unable to send file opened message");
        fclose(file);
        return;
    }

    if(!file) fclose(file);

    else if(receiveConfirmation(clientSocket)){
        perror("[-] Error UploadingFile(): Receiving confirmation on sent message");
        fclose(file);
    }

    else {
        char buffer[BUFFER_LENGTH] = {'\0'};
        while (fgets(buffer, BUFFER_LENGTH, file) != NULL){
            if (sendData(clientSocket, buffer)){
                perror("[-] Error sending file");
                fclose(file);
                return;
            }
        }

        if (sendData(clientSocket, "COMPLETE")){
            perror("[-] Error sending COMPLETE");
            fclose(file);
            return;
        }

        fclose(file);
        printf("File %s sent successfully.\n", filename);
    }
}


// Function to open a connection to a PORT and accept a single connection
int open_a_connection_port(int Port, int num_listener)
{
    int socketOpened = socket(AF_INET, SOCK_STREAM, 0);
    if (socketOpened < 0){
        perror("[-] Error open_a_connection_port: opening socket");
        return -1;
    }
    
    struct sockaddr_in serverAddress;
    memset(&serverAddress, 0, sizeof(serverAddress));

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(Port);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    // Binding the socket
    if(bind(socketOpened, (struct sockaddr *)&serverAddress, sizeof(serverAddress))<0){
        perror("[-] Error open_a_connection_port: binding socket");
        return -1;
    }

    // Listening for connection
    if (listen(socketOpened, num_listener) == -1){
        perror("[-] Error: Unable to listen");
        return -1;
    }

    return socketOpened;
}


// Function that connects and return the socket given IP, PORT, 0 otherwise
int connectToServer(const char* IP_address, const int PORT)
{
    int serverSocket;
    if((serverSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        perror("[-] Error connectToServer(): Connecting to Storage server");
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


// Function to send the specified data and wait for confirmation i.e. VALID_STRING
int sendDataAndReceiveConfirmation(int socket, char* data){
    if(sendData(socket, data)){
        return -1;
    }
    if(receiveConfirmation(socket)) {
        return -1;
    }
    return 0;
}