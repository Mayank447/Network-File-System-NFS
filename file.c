#include "storage_server.h"
#include "file.h"

// Some global variables
char File_Msg[BUFFER_LENGTH];
char error_message_file[ERROR_BUFFER_LENGTH];

File* fileHead = NULL;
File* fileTail = NULL;



//////////////////////////////// FILE STRUCT FUNCTIONS ///////////////////////

// Check if the file path exists (returns 1 is yes, otherwise returns 0)
int checkFilePathExists(char* path){
    File* ptr = fileHead;
    while(ptr != NULL) {
        if(strcmp(path, ptr->filepath)==0) return 1;
        ptr=ptr->next;
    }
    return 0;
}


// Check if the filepath is valid and stored in file struct
int validateFilePath(char* filepath, char* operation_no)
{
    /* Function also locks the file based on the operation number */
    File* ptr = fileHead;
    while(ptr != NULL) {
        if(strcmp(filepath, ptr->filepath)==0) break;
        ptr=ptr->next;
    }

    if(ptr == NULL) return ERROR_PATH_DOES_NOT_EXIST;
    
    if(strcmp(operation_no, READ_FILE)==0){
        pthread_mutex_lock(&ptr->read_write_lock);
            
        //Someone is already writing to the file
        if(ptr->read_write == 1) {
            pthread_mutex_unlock(&ptr->read_write_lock);
            return ERROR_A_CLIENT_ALREADY_WRITING_TO_FILE; 
        }
        else ptr->read_write = 0;
            
        // Incrementing the reader count
        pthread_mutex_lock(&ptr->get_reader_count_lock);
        ptr->reader_count++;
        printf("Count: %d\n", ptr->reader_count);
        pthread_mutex_unlock(&ptr->get_reader_count_lock);

        pthread_mutex_unlock(&ptr->read_write_lock);
        return VALID;
    }

    // File path matches and operation is to write
    else if(strcmp(operation_no, WRITE_FILE) == 0)
    {
        pthread_mutex_lock(&ptr->read_write_lock);
        
        // Someone is already reading the file
        if(ptr->read_write == 0){
            pthread_mutex_unlock(&ptr->read_write_lock);
            return ERROR_A_CLIENT_ALREADY_READING_THE_FILE;
        }
        else ptr->read_write = 1;

        pthread_mutex_unlock(&ptr->read_write_lock);
        return VALID;
    }

    else {
        return VALID;
    }
}


// Decrease the reader count of a particular file
void decreaseReaderCount(char* path)
{
    File* file = fileHead;
    while(file != NULL) {
        if(strcmp(path, file->filepath)==0)
        {
            pthread_mutex_lock(&file->read_write_lock);
            pthread_mutex_lock(&file->get_reader_count_lock);
            
            printf("Filename: %s", file->filepath);
            printf("Reader_count: %d", file->reader_count);
            if(file->read_write != 1 && --file->reader_count == 0){
                file->read_write = -1;
            }
            pthread_mutex_unlock(&file->get_reader_count_lock);
            pthread_mutex_unlock(&file->read_write_lock);
            return;
        }
        file = file->next;
    }
    return;
}


// Function to add a File to the file struct
int addFile(char* path, int check)
{
    // Check if the path is not already stored in the storage server
    if(check && checkFilePathExists(path)) {
        printf("FILE already exists in Accessible path list\n");
        return ERROR_FILE_ALREADY_EXISTS;
    }

    // Creating a new File struct if the path is not already stored
    File* newFile = (File*)malloc(sizeof(File));
    strcpy(newFile->filepath, path);
    newFile->read_write = -1;
    newFile->reader_count = 0;
    newFile->next = NULL;

    if(pthread_mutex_init(&newFile->get_reader_count_lock, NULL) < 0 ||
        pthread_mutex_init(&newFile->read_write_lock, NULL)< 0)
    {
        printf("[-] Error addFile(): Unable to initialize mutex lock\n");
        return STORAGE_SERVER_DOWN;
    }
    
    // Adding it to the File struct linked list
    if(fileHead == NULL){
        fileHead = newFile;
        fileTail = newFile;
        return 0;
    }

    fileTail->next = newFile;
    fileTail = newFile;
    return 0;
}


// Function to remove the file from file struct
int removeFile(char* path){
    File* ptr = fileHead, *ptr2;
    if(ptr == NULL) return 0;

    while(ptr->next != NULL) {
        if(strcmp(path, ptr->next->filepath)==0) {
            ptr2 = ptr->next;
            ptr->next = ptr2->next;
            free(ptr2);
            return 1;
        }
        ptr=ptr->next;
    }
    return 0;
}


// Function to clean up the file struct
void cleanUpFileStruct(){
    File* ptr = fileHead, *ptr2;
    if(ptr == NULL) return;

    while(ptr != NULL) {
        ptr2 = ptr->next;
        free(ptr);
        ptr=ptr2;
    }
}


// Opening the file lock from writing
void openWriteLock(char* path){
    File* file = fileHead;
    while(file != NULL) {
        if(strcmp(path, file->filepath) == 0){
            pthread_mutex_lock(&file->read_write_lock);
            file->read_write = -1;
            pthread_mutex_unlock(&file->read_write_lock);
            return;
        }
        file = file->next;
    }
}



////////////////////////////// ACTUAL FILE FUNCTIONS ////////////////////////////


// Check the existence of a path and whether it corresponds to a file/directory
// returns 0 in case of files and 1 in case of directories, -1 in case it does not exist
int checkFileType(char* path)
{ 
    struct stat path_stat;
    stat(path, &path_stat);

    if(S_ISREG(path_stat.st_mode)) return 0;
    else if(S_ISDIR(path_stat.st_mode)) return 1;
    return -1;
}


// Returns 1 is the file exists
int fileExists(char *filename) {
    return access(filename, F_OK) != -1;
}


// Function to create a File
void createFile(char* path, char* response)
{
    if(fileExists(path)){
        sprintf(response, "%d", ERROR_FILE_ALREADY_EXISTS);
    }

    else{
        FILE* file = fopen(path, "w");
        if(file == NULL){
            sprintf(response, "%d", ERROR_CREATING_FILE); 
        }
        else{
            fclose(file);
            addFile(path, 0);
            strcpy(response, VALID_STRING);
        }
    }
}


// Function to get the meta data(additional information) about the file
void getFileMetaData(char* filepath, int clientSocketID) 
{
    struct stat fileStat;
    char buffer[1000];

    // Use the stat function to retrieve file metadata
    if (stat(filepath, &fileStat) == 0) {
        
        // Format file metadata into a single string
        // filepath : filesize : file_permissions : last_access_time : last_modification_time : creation_time
        int n = sprintf(buffer, "%s:%ld:%o:%s:%s:%s", filepath, (long)fileStat.st_size, (unsigned int)(fileStat.st_mode & 0777), 
                        ctime(&fileStat.st_atime), ctime(&fileStat.st_mtime), ctime(&fileStat.st_ctime));
        
        if (n < 0) {
            perror("[-] Error formatting file metadata");
            sprintf(buffer, "%d", STORAGE_SERVER_ERROR);
        }
    }

    else {
        perror("[-] Stat: Unable to get file stat");
        sprintf(buffer, "%d", ERROR_GETTING_FILE_PERMISSIONS);
    }

    // Send the formatted metadata or error buffer
    if (sendData(clientSocketID, buffer)){
        printf("[-] Error sending file metadata\n");
    }
}


// Function to delete a File
void deleteFile(char *filename, char* response)
{
    // File does not exist
    if (access(filename, F_OK) != 0) { 
        sprintf(response, "%d", ERROR_FILE_DOES_NOT_EXIST);
        return;
    }
    else strcpy(response, VALID_STRING);

    // Check for permission [TODO]
    if(remove(filename) == 0) {
        strcpy(response, VALID_STRING);
        removeFile(filename);
    }
    else 
        sprintf(response, "%d", ERROR_UNABLE_TO_DELETE_FILE);
}


// Function to download a file
void DownloadFile(int serverSocket, char* filename)
{
    char buffer[BUFFER_LENGTH];
    if(nonBlockingRecv(serverSocket, buffer)) return;
    
    if(strcmp(buffer, VALID_STRING) != 0){
        printError(buffer);
        return;
    }

    if(sendConfirmation(serverSocket)) return;

    FILE* file = fopen(filename, "w");
    if(!file){
        printf("Unable to open the FILE %s for writing\n", filename);
        return;
    }

    // Receiving the FILE DATA
    while(1){
        int status = nonBlockingRecv(serverSocket, buffer);
        if(status){
            printf("Error downloadFile(): Unable to received file content");
            fclose(file);
            return;
        }

        if(sendConfirmation(serverSocket)){
            printf("Error downloadFile(): Error sending confirmation");
            fclose(file);
            return;
        }
        
        if(strstr(buffer, "COMPLETE") != NULL) {
            break;
        }

        else if(fprintf(file, "%s", buffer) < 0){
            printf("Error downloadFile(): Unable to write to the file");
            fclose(file);
            return;
        }
    }
    fclose(file);
    printf("File %s downloaded successfully.\n", filename);
}


// Function to upload a file
int UploadFile(int clientSocket, char* filename)
{
    // Open the file for reading on the server side
    FILE *file = fopen(filename, "r");
    bzero(error_message_file, ERROR_BUFFER_LENGTH);

    if (!file) sprintf(error_message_file, "%d", ERROR_OPENING_FILE);
    else sprintf(error_message_file, VALID_STRING);

    if(sendDataAndReceiveConfirmation(clientSocket, error_message_file)){
        perror("[-] Error UploadingFile(): Unable to send file opened message");
        fclose(file);
        return -1;
    }

    if(!file) fclose(file);

    else {
        char buffer[BUFFER_LENGTH] = {'\0'};
        while (fgets(buffer, BUFFER_LENGTH, file) != NULL){
            if (sendDataAndReceiveConfirmation(clientSocket, buffer)){
                perror("[-] Error sending file");
                fclose(file);
                return -1;
            }
        }

        if (sendData(clientSocket, "COMPLETE")){
            perror("[-] Error sending COMPLETE");
            fclose(file);
            return -1;
        }

        fclose(file);
        printf("File %s sent successfully.\n", filename);
    }
    return 0;
}


void copyFile(char* ss_path, char* response)
{
    char IP_address[20], path[MAX_PATH_LENGTH];
    int PORT = 0;
    if(sscanf(ss_path, "%[^:]:%d:%s", IP_address, &PORT, path) != 3){
        printf("[-] Error parsing the SS_Path string\n");
        return;
    }

    int copySocket = connectToServer(IP_address, PORT);
    
    if(sendDataAndReceiveConfirmation(copySocket, WRITE_FILE)){
        printf("[-] Error sending Copying Operation number");
        return;
    }

    if(sendDataAndReceiveConfirmation(copySocket, path)){
        printf("[-] Error sending Copying Operation number");
        return;
    }
    UploadFile(copySocket, path);
    close(copySocket);
}

void copyDirectory(char* ss_path, char* response){

}