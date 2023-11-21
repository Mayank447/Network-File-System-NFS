#include "header_files.h"
#include "storage_server.h"
#include "file.h"
#include "directory.h"

#define NS_PORT 9090 // PORT for initializing connection with the NameServer

//Global pointers for beggining and last file in struct linked list
extern File* fileHead;
extern File* fileTail;

int ss_id;      // Storage server ID
int clientPort; // PORT for communication with the client
int clientSocketID; //Main binded socket for accepting requests from clients

int nsSocketID; // Main socket for communication with the Name server
int nsPort;     // PORT for communication with Name server (user-specified)
char nsIP[16];  // Assuming IPv4

char SS_Msg[ERROR_BUFFER_LENGTH];
char paths_file[50] = ".paths_SS.txt";

/* Close the sockets*/
void closeConnection(){
    close(clientSocketID);
    close(nsSocketID);
    cleanUpFileStruct();
}

/* Signal handler in case Ctrl-Z or Ctrl-D is pressed -> so that the socket gets closed */
void handle_signal(int signum){
    closeConnection();
    exit(signum);
}


// Function to collect accessible paths from the user and store them in a file
// If paths_SS*.txt file already exists, then read the paths from there
void collectAccessiblePaths()
{
    char path[PATH_BUFFER_SIZE];
    char *filename = ".paths_SS*.txt";

    // Check if paths file exists
    if (fileExists(filename) )
    {    
        FILE *file = fopen(filename, "r");
        if(file){
            while (fgets(path, sizeof(path), file) != NULL)
            {
                int index = strchr(path, '\n') - path;
                path[index] = '\0';
        
                if(checkFileType(path) == 0) 
                    addFile(path, 1);
            }
            fclose(file);
            return;
        }
    }


    FILE *file = fopen(paths_file, "w");
    if (file == NULL){
        perror("[-] Error opening .paths_SS.txt");
        return;
    }

    while (1)
    {
        printf("Enter an accessible path (or 'exit' to stop): ");
        fgets(path, sizeof(path), stdin);
        if (strcmp(path, "exit\n") == 0) 
            break;
        else{
            int index = strchr(path, '\n') - path;
            path[index] = '\0';
        }
        
        int type = checkFileType(path);

        // Invalid path
        if(type == -1) 
            printf("Invalid path\n");

        // Path corresponds to a File
        else if(type == 0) {
            fprintf(file, "%s\n", path);
            addFile(path, 1); // Store the path in a File struct
        }
        
        // Path corresponds to a directory
        else if(type == 1){
            fprintf(file, "%s\n", path); // Write the path to the Path file 
        }

        else{
            printf("Internal server error\n");
        }
    }
    fclose(file);
}



/////////////////// FUNCTIONS FOR INITIALIZING THE CONNECTION WITH THE NAME SERVER /////////////////////
int sendInfoToNamingServer(const char *nsIP, int nsPort, int clientPort)
{
    /* Function to send vital information to the Naming Server and receive the ss_id */
    int nsSocket;
    struct sockaddr_in nsAddress;

    // Create a socket for communication with the Naming Server
    if ((nsSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        perror("[-] Error: opening socket for Naming Server");
        return -1;
    }
    
    memset(&nsAddress, 0, sizeof(nsAddress));
    nsAddress.sin_family = AF_INET;
    nsAddress.sin_port = htons(NS_PORT);
    nsAddress.sin_addr.s_addr = inet_addr(nsIP);
    
    // Connect to the Naming Server
    if (connect(nsSocket, (struct sockaddr *)&nsAddress, sizeof(nsAddress)) < 0){
        perror("[-] Error: connecting to Naming Server");
        close(nsSocket);
        return -1;
    }

    // Prepare the information to send
    char infoBuffer[PATH_BUFFER_SIZE], tempBuffer[PATH_BUFFER_SIZE];
    snprintf(infoBuffer, sizeof(infoBuffer), "SENDING|STORAGE|SERVER|INFORMATION");
    strcat(infoBuffer, ":");
    snprintf(tempBuffer, sizeof(infoBuffer), "%s;%d;%d", nsIP, nsPort, clientPort);
    strcat(infoBuffer, tempBuffer);
    strcat(infoBuffer, ":");

    // Open the file for reading
    FILE *pathFile = fopen(paths_file, "r");
    if (pathFile == NULL){
        perror("[-] Error opening path file");
        close(nsSocket);
        return -1;
    }

    // Read and concatenat each path specified
    char path[PATH_BUFFER_SIZE];
    while (fgets(path, sizeof(path), pathFile) != NULL)
    {
        if(path[strlen(path)-1] == '\n') path[strlen(path)-1] = '\0';
        strcat(infoBuffer, path);
        strcat(infoBuffer, ":");
    }

    // Concatenating a "COMPLETED" message
    const char *completedMessage = "COMPLETED";
    strcat(infoBuffer, completedMessage);

    // Sending the information buffer and closing the file descriptor
    if (send(nsSocket, infoBuffer, strlen(infoBuffer), 0) < 0){
        perror("[-] Error: sending information to Naming Server");
        fclose(pathFile);
        close(nsSocket);
        return -1;
    }
    fclose(pathFile);

    //  Connecting a Storage Server ID from the Naming Server on the user input 
    if((nsSocketID = (open_a_connection_port(nsPort, 1))) == -1){
        printf("Error: opening a dedicated socket for communication with NameServer");
        return -1;
    }

    // Receiving the Storage Server ID from NameServer
    char responseBuffer[PATH_BUFFER_SIZE];
    if (recv(nsSocket, responseBuffer, sizeof(responseBuffer), 0) < 0){
        perror("[-] Error: receiving Storage Server ID from Naming Server");
        return -1;
    }
    close(nsSocket);

    ss_id = atoi(responseBuffer);
    printf("[+] Assigned Storage Server ID: %d\n", ss_id);
    printf("[+] Storage Server connected to Nameserver on PORT %d ...\n", nsPort);
    return 0;
}



// Function to maintain a heart beat/pulse with the Name server
void* NameServerPulseHandler()
{
    // Waiting for connection request from NameServer
    int nsSocket = accept(nsSocketID, NULL, NULL);
    if (nsSocket < 0){
        perror("[-] Name Server accept failed...");
        return NULL;
    }

    char buffer[BUFFER_LENGTH];
    int not_received_count = 0;

    while(1 && not_received_count < NOT_RECEIVED_COUNT)
    {
        bzero(buffer, BUFFER_LENGTH);
        if(sendData(nsSocket, "DOWN")){;}
        sleep(PERIODIC_HEART_BEAT);

        if(nonBlockingRecvPeriodic(nsSocket, buffer)) {
            not_received_count++;
            continue;
        }
        not_received_count = 0;
        if(strcmp(buffer, "UP") != 0) break;
    }
    printf("[-] Connection with Nameserver is broken\n");
    close(nsSocket);
    return NULL;
}



//////////////// FUNCTIONS TO HANDLE CLIENT BASED COMMUNICATION WITH NAMESERVER ///////////////////
void NameServerThreadHandler()
{
    while(1){
        int nsSocket = accept(nsSocketID, NULL, NULL);
        if (nsSocket < 0) {
            perror("[-] Error NameServerThreadHandler(): Nameserver thread accept failed");
            continue;
        }
        printf("[+] Nameserver thread connection request accepted\n");
        
        // Create a new thread for each new Nameserver thread
        pthread_t nsThread;
        if (pthread_create(&nsThread, NULL, (void*)handleNameServerThread, (void*)&nsSocket) < 0) {
            perror("[-] Thread creation failed");
            continue;
        }

        // Detach the thread
        if (pthread_detach(nsThread) != 0) {
            perror("[-] Error detaching Nameserver thread");
            continue;
        }
    }
    return;
}


// Function for communication between NS and SS for client functions
void* handleNameServerThread(void* args)
{
    int nsSocket = *(int*)args;
    
    // Receive the operation number and based on that path
    int op = receiveOperationNumber(nsSocket);
    if(op == -1) {
        close(nsSocket);
        return NULL;
    }  

    // Receiving and sending confirmation for the first path
    char path[BUFFER_LENGTH], response[100];
    if(receivePath(nsSocket, path)){
        close(nsSocket);
        return NULL;
    }

    // Create File
    if(op == atoi(CREATE_FILE)){
        createFile(path, response);
        if(sendData(nsSocket, response)){
            printf("[-] Error sending createFile() response to Name server\n");
        }
    }

    // Create Folder
    else if(op == atoi(CREATE_DIRECTORY)){
        createDirectory(path, response);
        if(sendData(nsSocket, response)){
            printf("[-] Error sending createDirectory() response to Name server\n");
        }
    }

    // Delete File
    else if(op == atoi(DELETE_FILE)){
        deleteFile(path, response);
        if(sendData(nsSocket, response)){
            printf("[-] Error sending deleteFile() response to Name server\n");
        }
    }

    // Delete Directory
    else if(op == atoi(DELETE_DIRECTORY)){
        deleteDirectory(path, response);
        if(sendData(nsSocket, response)){
            printf("[-] Error sending deleteDirectory() response to Name server\n");
        }
    }

    // Copy File
    else if(op == atoi(COPY_FILES)){
        printf("Inside\n");
        printf("Path: %s\n", path);
        copyFile(path, response);
        if(sendData(nsSocket, response)){
            printf("[-] Error sending deleteFile() response to Name server\n");
        }
    }

    // Copy Directory
    else if(op == atoi(COPY_DIRECTORY)){
        copyDirectory(path, response);
        if(sendData(nsSocket, response)){
            printf("[-] Error sending deleteDirectory() response to Name server\n");
        }
    }
    
    close(nsSocket);
    return NULL;
}




////////////////////// FUNCTIONS TO HANDLE COMMUNICATION WITH CLIENT /////////////////////////
void handleClients()
{
    struct sockaddr_in client_address;
    socklen_t address_size = sizeof(struct sockaddr_in);
    
    while(1){
        int clientSocket = accept(clientSocketID, (struct sockaddr*)&client_address, &address_size);
        if (clientSocket < 0) {
            perror("[-] Error handleClients(): Client accept failed");
            continue;
        }
        
        printf("[+] Client connection request accepted from %s:%d\n", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));
        
        // Create a new thread for each new client
        pthread_t clientThread;
        if (pthread_create(&clientThread, NULL, (void*)handleClientRequest, (void*)&clientSocket) < 0) {
            perror("[-] Thread creation failed");
            continue;
        }

        // Detach the thread
        if (pthread_detach(clientThread) != 0) {
            perror("[-] Error detaching client thread");
            continue;
        }
    }
    return;
}


// Function (thread function) to handle individual client request
void* handleClientRequest(void* argument)
{
    // Receiving and validating the Operation number from the client
    int clientSocket = *(int*)argument;
    int request_no = receiveOperationNumber(clientSocket);
    if(request_no == -1) return NULL;
    
    char filepath[MAX_PATH_LENGTH];


    //READ FILE
    if(request_no == atoi(READ_FILE)) {
        if(receive_ValidateFilePath(clientSocket, filepath, READ_FILE, 1) == 0){
            UploadFile(clientSocket, filepath);
            decreaseReaderCount(filepath);
        }
    }

    // WRITE FILE
    else if(request_no == atoi(WRITE_FILE)){
        if(receive_ValidateFilePath(clientSocket, filepath, WRITE_FILE, 1) == 0){
            DownloadFile(clientSocket, filepath);
            openWriteLock(filepath);
        }
    }

    // GET PERMISSIONS
    else if(request_no == atoi(GET_FILE_PERMISSIONS)){
        if(receive_ValidateFilePath(clientSocket, filepath, GET_FILE_PERMISSIONS, 1) == 0){
            getFileMetaData(filepath, clientSocket);
        }
    }

    // COPY FILES
    else if(request_no == atoi(COPY_FILES)){
        if(receive_ValidateFilePath(clientSocket, filepath, COPY_FILES, 0) == 0){
            DownloadFile(clientSocket, filepath);
        }
    }

    close(clientSocket);
    return NULL;
}



// Function to receive the "file" path from the client
int receive_ValidateFilePath(int clientSocket, char* filepath, char* operation_no, int check)
{
    // Receiving the file path
    if(nonBlockingRecv(clientSocket, filepath)){
        perror("[-] Error receive_ValidateFilePath(): Unable to receive the file path");
        return -1;
    } 

    // Validating the filepath based on return value (ERROR_CODE)
    int valid = 0;
    if(check) {
        valid = validateFilePath(filepath, operation_no);
    }

    char response[100];
    sprintf(response, "%d", valid);

    //Sending back the response
    if(sendData(clientSocket, response)){
        perror("[-] Error receive_validateRequestNo(): Unable to send reply to Path sent");
        return -1;
    }
    return valid;
}



/////////////////////////// MAIN FUNCTION ///////////////////////////
int main(int argc, char *argv[])
{
    // Signal handler for Ctrl+C and Ctrl+Z
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);
    
    // Ask the user for the Naming Server's IP and port and Client communication Port
    printf("Enter the IP address of the Naming Server: ");
    scanf("%s", nsIP);
    printf("Enter the port to talk with the Naming Server: ");
    scanf("%d", &nsPort);
    printf("Enter the port to talk with the Client: ");
    scanf("%d", &clientPort);
    getchar(); //For linux users
    fflush(stdin);

    // Collect accessible paths from the user and store in a file
    collectAccessiblePaths();
    
    // Send vital information to the Naming Server and receive the Storage Server ID
    int initialiazed = sendInfoToNamingServer(nsIP, nsPort, clientPort);
    if (initialiazed == -1) {
        printf("[-] Failed to send information to the Naming Server.\n");
    }
    else{
        char filename[50];
        sprintf(filename, ".paths_SS%d.txt", ss_id);
        if (rename(paths_file, filename) != 0){
            perror("[-] Error renaming the Paths file");
        }
    }

    // Create a thread to communicate with the nameserver
    pthread_t NameServerThread, NameServerPulseThread;;
    pthread_create(&NameServerPulseThread, NULL, (void*)&NameServerPulseHandler, NULL);
    pthread_create(&NameServerThread, NULL, (void*)&NameServerThreadHandler, NULL);

    // Accepting request from clients - This will loop for ever
    clientSocketID = open_a_connection_port(clientPort, MAX_CLIENT_CONNECTIONS);
    printf("[+] Storage server listening for clients on PORT %d\n", clientPort);
    handleClients();
    
    // Closing connection - This part of the code is never reached
    pthread_join(NameServerThread, NULL);
    closeConnection();
    return 0;
}
