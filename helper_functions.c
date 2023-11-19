#include "header_files.h"
#include "helper_functions.h"

char error_message[1024];

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
    if(recv(args->serverSocket, args->buffer, sizeof(args->buffer), 0) < 0){
        pthread_testcancel();
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
int createRecvThread(int serverSocket, char* buffer)
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


// Thread to send response
int sendReponse(int socket, char* response){
    if(send(socket, response, strlen(response), 0) < 0){
        perror("[-] sendConfirmation(): Error sending confirmation");
        return -1;
    }
    return 0;
}

// Thread to send confirmation of valid
int sendConfirmation(int socket){
    if(send(socket, VALID_STRING, strlen(VALID_STRING), 0) < 0){
        perror("[-] sendConfirmation(): Error sending confirmation");
        return -1;
    }
    return 0;
}

// Wrapper around CreateRecvThread to check for confirmation
int receiveConfirmation(int serverSocket, char* buffer)
{
    if(createRecvThread(serverSocket, buffer)) 
        return -1;
    
    if(strcmp(buffer, VALID_STRING) != 0) {
        printf("[-] %s\n", buffer);
        return -1;
    }
    return 0;
}

// Checking the operation number
int checkOperationNumber(char* buffer)
{
    int num = atoi(buffer);
    bzero(buffer, BUFFER_LENGTH);
    if(atoi(buffer) < 0 || atoi(buffer) > 9) return -1;
    return num;
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
    int bytesReceived;
    char buffer[BUFFER_LENGTH];
    if(recv(socket, buffer, 2, 0) < 0){
        perror("[-] Error downloadFile(): Unable to receive the file open confirmation");
        fclose(file);
        return;
    }

    if(atoi(buffer) != 0){
        bzero(error_message, 1024);
        handleErrorCodes(buffer, error_message);
        printf("Error: %s\n", error_message);
    }

    // Receiving the FILE DATA
    while(1)
    {
        bytesReceived = recv(socket, buffer, sizeof(buffer), 0);
        if(bytesReceived == 0) break;
        if(bytesReceived < 0){
            perror("[-] Error downloadFile(): Unable to receive the file data");
            break;
        }

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

    if (!file) sprintf(error_message, "13");
    else sprintf(error_message, "0");

    if (send(clientSocket, error_message, strlen(error_message), 0) < 0){
        perror("[-] Error UploadingFile(): Unable to send file opened message");
    }

    else {
        char buffer[1024] = {'\0'};
        while (fgets(buffer, sizeof(buffer), file) != NULL){
            if (send(clientSocket, buffer, strlen(buffer), 0) < 0){
                perror("[-] Error sending file");
                fclose(file);
                return;
            }
        }

        fclose(file);
        printf("File %s sent successfully.\n", filename);
    }
}
