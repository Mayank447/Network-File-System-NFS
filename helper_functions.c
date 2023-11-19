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


int createRecvThread(int serverSocket)
{
    char buffer[BUFFER_LENGTH];
    struct ReceiveThreadArgs args;
    args.buffer = buffer;
    args.serverSocket = serverSocket;

    pthread_t receiveThread;
    pthread_create(&receiveThread, NULL, (void*)receiveConfirmation, (void*)&args);
    clock_t start_time = clock();
    while(((double)(clock() - start_time))/ CLOCKS_PER_SEC  <  RECEIVE_THREAD_RUNNING_TIME);
    
    if(pthread_join(receiveThread, NULL) == 0){
        char message[BUFFER_LENGTH];
        handleErrorCodes(buffer, message);
        return atoi(buffer);
    } 
    else
        printf("Failed to receive confirmation from the thread\n");
    
    return -1;
}


void* receiveConfirmation(int serverSocket, char* buffer)
{
    // Receiving the confirmation for storage server's side
    if(recv(serverSocket, buffer, sizeof(buffer), 0) < 0){
        perror("[-] Error createFile(): Unable to receive the confirmation from server's side");
    }
    return NULL;
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
