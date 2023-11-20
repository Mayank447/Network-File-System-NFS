#include "header_files.h"
#include "name_server.h"
#include "params.h"

int clientSocket, storageServerSocket; // Sockets for handling storage server queries and client requests
pthread_t* storageServerThreads;       //Thread for direct communication with a Storage server
pthread_t* clientThreads;              //Thread for direct communication with a client

// Arguments to send while creating a Storage server thread
struct storageServerArg{
    int socket;
    int ss_id;
};

// Function to close server and client connection sockets
void closeConnections(){ 
    close(clientSocket);
    close(storageServerSocket);
    cleanStorageServerInfoLinkedList();
}

/* Signal handler in case Ctrl-C or Ctrl-D is pressed -> so that the socket gets closed */
void handle_signal(int signum) {
    closeConnections();
    exit(signum);
}


///////////////////////////// STORAGE SERVER INITIALIZATION/////////////////////////////
int ss_count = 0;                                                   // Count of no. of storage servers
pthread_mutex_t ss_count_lock = PTHREAD_MUTEX_INITIALIZER;          // Lock for the above

struct StorageServerInfo *storageServerList = NULL;                 // Head of linked list(reversed) of storage server
pthread_mutex_t storageServerHead_lock = PTHREAD_MUTEX_INITIALIZER; // Lock for the above


// Function to add storage server (meta) information to the linked list
struct StorageServerInfo* addStorageServerInfo(const char *ip, int ns_port, int cs_port)
{
    struct StorageServerInfo *newServer = (struct StorageServerInfo *)malloc(sizeof(struct StorageServerInfo));
    strcpy(newServer->ip_address, ip);
    newServer->naming_server_port = ns_port;
    newServer->client_server_port = cs_port;
    newServer->ss_id = ss_count;
    newServer->count_accessible_paths = 0;

    for (int i = 0; i < MAX_NO_ACCESSIBLE_PATHS; i++){
        newServer->accessible_paths[i][0] = '\0';
    }

    newServer->redundantSS_1 = NULL;
    newServer->redundantSS_2 = NULL;
    newServer->running = 1;

    pthread_mutex_lock(&storageServerHead_lock);
    newServer->next = storageServerList; //Reversed linked list 
    storageServerList = newServer;
    pthread_mutex_unlock(&storageServerHead_lock);
    
    return newServer;
}


// Thread handler for storage server request/queries (General/open handler)
void* handleStorageServerInitialization()
{   
    struct storageServerArg args[MAX_SERVERS];

    while(1){
        struct sockaddr_in server_address;
        socklen_t address_size = sizeof(struct sockaddr_in);
        int serverSocket = accept(storageServerSocket, (struct sockaddr*)&server_address, &address_size);
        if (serverSocket < 0) {
            perror("[-] Accept failed");
            continue;
        }

        printf("[+] Storage Server Connection accepted from %s:%d\n", inet_ntoa(server_address.sin_addr), ntohs(server_address.sin_port));
        
        // Create a new thread for each storage server
        args[ss_count].socket = serverSocket;
        args[ss_count].ss_id = ss_count + 1;

        pthread_mutex_lock(&ss_count_lock);
        if (pthread_create(&storageServerThreads[ss_count], NULL, (void*)handleStorageServer, (void*)&args[ss_count]) < 0) {
            perror("[-] Thread creation failed");
            pthread_mutex_unlock(&ss_count_lock);
            continue;
        }

        // Detach the thread - resources of the thread can be released when it finishes without waiting for it explicitly.
        if (pthread_detach(storageServerThreads[ss_count]) != 0) {
            perror("[-] Error detaching Storage server thread");
            continue;
        }

        ss_count++;
        pthread_mutex_unlock(&ss_count_lock);
    }
    return NULL;
}


// Individual thread handler for a server (Connection initialization + heart beat)
void* handleStorageServer(void* argument)
{
    struct storageServerArg* args = (struct storageServerArg*)argument;
    int connectedServerSocketID = args->socket;
    int serverID = args->ss_id;
    struct StorageServerInfo* server = NULL;

    int naming_server_port, client_server_port;
    char ip_address[256], buffer[BUFFER_LENGTH];
    int receivingInfo = 0; // State variable
    
    // Initialization process
    int bytesReceived = recv(connectedServerSocketID, buffer, BUFFER_LENGTH, 0);
    if (bytesReceived < 0){
        perror("[-] Error in Initializing the server");
        return NULL;
    }

    buffer[bytesReceived] = '\0';
    trim(buffer); // Removes leading white spaces

    char *token = strtok(buffer, ":");
    while (token != NULL)
    {
        // Checking if the string being sent is COMPLETED i.e. all the data was previously sent
        if (receivingInfo > 1 && strcmp(token, "COMPLETED") == 0)
        {
            // Sending the ss_id to the storage-server
            char ss_id_str[5];
            snprintf(ss_id_str, sizeof(ss_id_str), "%d", serverID); // Using snprintf to convert the integer to a string
            
            if (send(connectedServerSocketID, ss_id_str, strlen(ss_id_str), 0) < 0){
                perror("[-] Error sending ss_id");
                return NULL;
            }
            break;
        }

        // Check if the received message is "SENDING STORAGE SERVER INFORMATION"
        else if (strcmp(token, "SENDING|STORAGE|SERVER|INFORMATION") == 0){
            receivingInfo = 1;
        }

        // Check if we are in the state of receiving storage server information
        else if (receivingInfo == 1){
            parseStorageServerInfo(token, ip_address, &naming_server_port, &client_server_port); 
            server = addStorageServerInfo(ip_address, naming_server_port, client_server_port); // Add the information to the linked list
            receivingInfo = 2; //Ready to receive all the accessible paths
        }

        // Receiving accessible paths
        else if (receivingInfo == 2){
            pthread_mutex_lock(&server->count_accessible_path_lock);
            if((server->count_accessible_paths >= 0 && server->count_accessible_paths < MAX_NO_ACCESSIBLE_PATHS)){
                strcpy(server->accessible_paths[server->count_accessible_paths++], token);
            }
            pthread_mutex_unlock(&server->count_accessible_path_lock);
        }

        token = strtok(NULL, ":");
    }
    close(connectedServerSocketID);


    // Initializing the connection to specified Storage Server's PORT number
    int serverSocket = initConnectionToStorageServer(server);
    if(serverSocket != -1){
        server->serverSocket = serverSocket;
        printf("[+] Storage server %d initialization done.\n", serverID);
    }
    else{
        printf("[-] Error connecting to specificied PORT. This error needs to be handled.\n");
        return NULL;
    }

    // Check is the storage server is still up and running by sending a pulse every second
    int not_received_count = 0;
    server->running = 1;

    while(1 && not_received_count < NOT_RECEIVED_COUNT){
        bzero(buffer, BUFFER_LENGTH);

        if(sendData(serverSocket, "DOWN")) continue;
        sleep(PERIODIC_HEART_BEAT);

        if(createRecvThreadPeriodic(serverSocket, buffer)) {
            not_received_count++;
            continue;
        }
        not_received_count = 0;
        if(strcmp(buffer, "UP") != 0) break;
    }
    server->running = 0;
    printf("Storage server %d is down\n", serverID);
    close(serverSocket);
    return NULL;
}


// Function to initialize the connection to storage servers with the specified PORT number
int initConnectionToStorageServer(struct StorageServerInfo* server)
{
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if(serverSocket == -1) {
        perror("[-] Error in opening a socket");
        return -1;
    }

    struct sockaddr_in storageServerAddr;
    storageServerAddr.sin_family = AF_INET;
    storageServerAddr.sin_port = htons(server->naming_server_port);
    storageServerAddr.sin_addr.s_addr = INADDR_ANY;
    
    if (inet_pton(AF_INET, "127.0.0.1", &storageServerAddr.sin_addr.s_addr) < 0){
        perror("[-] Invalid address/Address not supported");
        close(serverSocket);
        return -1;
    }

    if (connect(serverSocket, (struct sockaddr *)&storageServerAddr, sizeof(storageServerAddr)) < 0){
        perror("[-] Error in connecting to the storage server");
        close(serverSocket);
        return -1;
    }

    return serverSocket; // Return the connected socket
}


// Function to parse the storage server parameters  [Need to check for validity of the sent data i.e. no. of digits]
void parseStorageServerInfo(const char *data, char *ip_address, int *ns_port, int *cs_port)
{
    // Parsing format - "IP:PORT1:PORT2", PORT 1 is used for communication with the name server and PORT2 with client.
    if (sscanf(data, "%[^;];%d;%d", ip_address, ns_port, cs_port) != 3)
    {
        printf("[-] Error parsing storage server info: %s\n", data);
    }
}


// Function to add an accessible path
int addAccessiblePath(int ss_id, char* path)
{
    pthread_mutex_lock(&storageServerHead_lock);
    struct StorageServerInfo* ptr = storageServerList;
    pthread_mutex_unlock(&storageServerHead_lock);
    
    while(ptr != NULL){
        pthread_mutex_lock(&ptr->count_accessible_path_lock);
        if(ptr->ss_id == ss_id && ptr->count_accessible_paths < MAX_NO_ACCESSIBLE_PATHS)
        {
            for(int j=0; j<MAX_NO_ACCESSIBLE_PATHS; j++){
                if(ptr->accessible_paths[j][0] == '\0'){
                    strcpy(ptr->accessible_paths[j], path);
                    ptr->count_accessible_paths++;
                    pthread_mutex_unlock(&ptr->count_accessible_path_lock);
                    return 0;
                }
            }
            pthread_mutex_unlock(&ptr->count_accessible_path_lock);
            return -1;
        }
        pthread_mutex_unlock(&ptr->count_accessible_path_lock);
        ptr = ptr->next;
    } 
    return -1;
}


// Function to delete an accessible path
int deleteAccessiblePath(int ss_id, char* path)
{
    pthread_mutex_lock(&storageServerHead_lock);
    struct StorageServerInfo* ptr = storageServerList;
    pthread_mutex_unlock(&storageServerHead_lock);
    
    while(ptr != NULL){
        pthread_mutex_lock(&ptr->count_accessible_path_lock);
        if(ptr->ss_id == ss_id && ptr->count_accessible_paths < MAX_NO_ACCESSIBLE_PATHS)
        {
            for(int j=0; j<MAX_NO_ACCESSIBLE_PATHS; j++){
                if(strcmp(ptr->accessible_paths[j], path) == 0){
                    ptr->accessible_paths[j][0] = '\0';
                    ptr->count_accessible_paths--;
                    pthread_mutex_unlock(&ptr->count_accessible_path_lock);
                    return 0;
                }
            }
            pthread_mutex_unlock(&ptr->count_accessible_path_lock);
            return -1;
        }
        pthread_mutex_unlock(&ptr->count_accessible_path_lock);
        ptr = ptr->next;
    } 
    return -1;
}


// Function to retrieve the storage server with minimum no. of accessible paths
struct StorageServerInfo* minAccessiblePathSS()
{
    pthread_mutex_lock(&storageServerHead_lock);
    struct StorageServerInfo *ptr = storageServerList, *server = NULL;
    pthread_mutex_unlock(&storageServerHead_lock);

    if(ptr == NULL) 
        return NULL;

    // Traversing through all storage servers
    int min_accessible_paths = MAX_NO_ACCESSIBLE_PATHS;

    while(ptr != NULL){
        pthread_mutex_lock(&ptr->count_accessible_path_lock);

        if(ptr->count_accessible_paths < min_accessible_paths){
            min_accessible_paths = ptr->count_accessible_paths;
            server = ptr;
        }
        pthread_mutex_unlock(&ptr->count_accessible_path_lock);
        ptr = ptr->next;
    } 
    return server;
}


// Function to delete the StorageServerInfo linked list
void cleanStorageServerInfoLinkedList()
{
    pthread_mutex_lock(&storageServerHead_lock);

    struct StorageServerInfo* server = storageServerList, *ptr;
    while(server != NULL){
        pthread_mutex_destroy(&server->count_accessible_path_lock);
        ptr = server->next;
        free(server);
        server = ptr;
    }

    pthread_mutex_unlock(&storageServerHead_lock);
}




/////////////////////////////// FUNCTION TO HANDLE CLIENTS REQUESTS/QUERIES ///////////////////////////////////

// Function to handle a new client request. The function assign a thread to each accepted client. */
void* handleClients()
{
    while(1){
        int newClientSocket = accept(clientSocket, NULL, NULL);
        if (newClientSocket < 0) {
            perror("[-] Error handleClients(): Client accept failed");
            continue;
        }      

        // Create a new thread for each new client
        pthread_t clientThread;
        if (pthread_create(&clientThread, NULL, (void*)handleClientRequests, (void*)&newClientSocket) < 0) {
            perror("[-] Error handleClients(): Thread creation failed");
            close(newClientSocket);
            continue;
        }

        // Detach the thread
        if (pthread_detach(clientThread) != 0) {
            perror("[-] Error handleClients(): Error in detaching client thread");
            close(newClientSocket);
            continue;
        }
    }
    return NULL;
}

// Function to handle a client request
void* handleClientRequests(void* socket)
{
    int clientSocket = *(int*)socket;
    char path[BUFFER_LENGTH];
    char response[BUFFER_LENGTH];
    
    // Receiving the operation number
    int op = receiveOperationNumber(clientSocket);
    if(op == -1){
        close(clientSocket);
        return NULL;
    }

    // Receiving the path
    if(createRecvThread(clientSocket, path)){
        close(clientSocket);
        return NULL;
    }

    // Any path checking if there (TODO)

    // Sending the path confirmation to Client
    if(sendConfirmation(clientSocket)){
        printf("[-] Unable to send the confirmation for the path\n");
        close(clientSocket);
        return NULL;
    }

    // Creating files
    if(op == atoi(CREATE_FILE))
    {        
        functionHandler(path, response, CREATE_FILE);
        if(sendData(clientSocket, response)) {
            printf("[-] Unable to send the createFile response back to client.\n");
        }
    }

    // Creating directory
    else if(op == atoi(CREATE_DIRECTORY))
    {        
        functionHandler(path, response, CREATE_DIRECTORY);
        if(sendData(clientSocket, response)) {
            printf("[-] Unable to send the createDirectory response back to client.\n");
        }
    }

    // Deleting files
    else if(op == atoi(DELETE_FILE))
    {        
        functionHandler(path, response, DELETE_FILE);
        if(sendData(clientSocket, response)) {
            printf("[-] Unable to send the deleteFile response back to client.\n");
        }
    }

    // Delete Folder
    else if(op == atoi(DELETE_DIRECTORY))
    {        
        functionHandler(path, response, DELETE_DIRECTORY);
        if(sendData(clientSocket, response)) {
            printf("[-] Unable to send the deleteDirectory response back to client.\n");
        }
    }

    // Copy files
    else if(op == atoi(COPY_FILES)){ 
        char path2[BUFFER_LENGTH];
        if(createRecvThread(clientSocket, path2)){
            close(clientSocket);
            return NULL;
        }

        copyHandler(path, path2, response, COPY_FILES);
        if(sendData(clientSocket, response)) {
            printf("[-] Unable to send the createFile response back to client.\n");
        }
    }

    // Copy Folders
    else if(op == atoi(COPY_DIRECTORY)){ 
        char path2[BUFFER_LENGTH];
        if(createRecvThread(clientSocket, path2)){
            close(clientSocket);
            return NULL;
        }

        copyHandler(path, path2, response, COPY_DIRECTORY);
        if(sendData(clientSocket, response)) {
            printf("[-] Unable to send the createFile response back to client.\n");
        }
    }

    close(clientSocket);
    return NULL;
}


// Function to create/delete File or Folder inside a storage server
void functionHandler(char* path, char* response, char* type)
{
    struct StorageServerInfo* server;

    // If the command is Create File/Directory
    if(strcmp(type, CREATE_FILE)==0 || strcmp(type, CREATE_DIRECTORY)==0)
        server = minAccessiblePathSS();

    // If the command is Delete File/Directory
    if(strcmp(type, DELETE_FILE)==0 || strcmp(type, DELETE_DIRECTORY)==0)
        server = searchStorageServer(path);
    
    if(server == NULL) {
        sprintf(response, "%d", ERROR_PATH_DOES_NOT_EXIST);
        return;
    }

    // Need to handle redundancy here [Check if the path already exists]

    // Connecting to the server
    int serverSocket = connectToServer(server->ip_address, server->naming_server_port);
    if(serverSocket == -1){
        sprintf(response, "%d", ERROR_PATH_DOES_NOT_EXIST);
        return;
    }

    // Sending the operation number and receiving the confirmation for it from the storage server
    if(sendDataAndReceiveConfirmation(serverSocket, type)){
        sprintf(response, "%d", STORAGE_SERVER_ERROR);
        close(serverSocket);
        return;
    }

    // Sending the file path and receiving the confirmation for it from the storage server
    if(sendDataAndReceiveConfirmation(serverSocket, path)){
        sprintf(response, "%d", STORAGE_SERVER_ERROR);
        close(serverSocket);
        return;
    }

    // Receiving the response (status of operation)
    if(createRecvThread(serverSocket, response)) {
        sprintf(response, "%d", STORAGE_SERVER_ERROR);
        close(serverSocket);
        return;
    }

    // Adding this path to the array of accessible paths
    if(strcmp(response, VALID_STRING)==0 && 
        (strcmp(type, CREATE_FILE)==0 || (strcmp(type, CREATE_DIRECTORY)==0)))
    {
        addAccessiblePath(server->ss_id, path);
    }

    // Remove this path if the operation is DELETE_FILE or DELETE_DIRECTORY
    else if(strcmp(response, VALID_STRING)==0 &&
        (strcmp(type, DELETE_FILE)==0 || (strcmp(type, DELETE_DIRECTORY)==0)))
    {
        deleteAccessiblePath(server->ss_id, path);
    }

    close(serverSocket);
}


// Function to copy files/folders between two paths
void copyHandler(char* path1, char* path2, char* response, char* op)
{
    struct StorageServerInfo *server1 = NULL, *server2 = NULL;
    if(strcmp(op, DELETE_FILE)==0 || strcmp(op, DELETE_DIRECTORY)==0){
        server1 = searchStorageServer(path1);
        server2 = searchStorageServer(path1);
    }

    // If one of the path doesn't exist exit
    if(!server1 || !server2){
        sprintf(response, "%d", ERROR_ONE_PATH_DOES_NOT_EXIST);
        return;
    }

    // Connecting to the server
    int serverSocket = connectToServer(server2->ip_address, server2->naming_server_port);
    if(serverSocket == -1){
        sprintf(response, "%d", ERROR_PATH_DOES_NOT_EXIST);
        return;
    }

    // Sending the operation number and receiving the confirmation for it from the storage server
    if(sendDataAndReceiveConfirmation(serverSocket, op)){
        sprintf(response, "%d", STORAGE_SERVER_ERROR);
        close(serverSocket);
        return;
    }

    char sendArg[MAX_PATH_LENGTH], temp[10];
    strcpy(sendArg, server1->ip_address);
    strcat(sendArg, ":");
    sprintf(temp, "%d", server1->client_server_port);
    strcat(sendArg, temp);
    strcat(sendArg, ":");
    strcat(sendArg, path1);
    strcat(sendArg, ":");
    strcat(sendArg, path2);

    // Sending the argument and receiving the confirmation for it from the storage server
    if(sendDataAndReceiveConfirmation(serverSocket, sendArg)){
        sprintf(response, "%d", STORAGE_SERVER_ERROR);
        close(serverSocket);
        return;
    }

    // Receiving the status of operation
    if(createRecvThread(serverSocket, response)) {
        sprintf(response, "%d", STORAGE_SERVER_ERROR);
        close(serverSocket);
        return;
    }

    // Adding this path to the array of accessible paths
    if(strcmp(response, VALID_STRING)==0){
        addAccessiblePath(server2->ss_id, path1);
    }
}




/////////////////////////// FUNCTIONS TO HANDLE STORAGE SERVER QUERYING /////////////////////////

// Function to search for a path in storage servers */
struct StorageServerInfo* searchStorageServer(char* file_path) 
{
    int found = 0;
    pthread_mutex_lock(&storageServerHead_lock);
    struct StorageServerInfo* temp = storageServerList;
    pthread_mutex_unlock(&storageServerHead_lock);
    
    // Search accessible paths in the current storage server [Linear Search]
    while (temp != NULL) {
        int i = 0;
        while (temp->accessible_paths[i][0] != '\0') 
        { 
            if (strcmp(file_path, temp->accessible_paths[i++]) == 0) {
                found = 1;
                break; 
            }
        }
        if(found) break;
        temp = temp->next;
    }

    if (found) return temp; // Found the path in a storage server
    else return NULL; // Path not found in any storage server
}


#define HASH_TABLE_SIZE 1000

// Define a struct for key-value pairs
struct KeyValue
{
    char *key;
    int value;
    struct KeyValue *next;
    struct KeyValue *prev; // For LRU
};

// Define the hashmap structure
struct LRU_HashMap
{
    struct KeyValue *table[HASH_TABLE_SIZE];
    struct KeyValue *lru_head; // LRU head
    struct KeyValue *lru_tail; // LRU tail
    int size;
};



// Hash function
int hash(char *key)
{
    int hash = 0;
    for (int i = 0; key[i]; i++)
    {
        hash = (hash * 31 + key[i]) % HASH_TABLE_SIZE;
    }
    return hash;
}

// Create a new hashmap
struct LRU_HashMap *createHashMap()
{
    struct LRU_HashMap *map = (struct LRU_HashMap *)malloc(sizeof(struct LRU_HashMap));
    for (int i = 0; i < HASH_TABLE_SIZE; i++)
    {
        map->table[i] = NULL;
    }
    map->lru_head = NULL;
    map->lru_tail = NULL;
    map->size = 0;
    return map;
}

// Insert a key-value pair into the hashmap, enforcing a capacity of 20
void insert(struct LRU_HashMap *map, char *key, int value)
{
    int index = hash(key);
    struct KeyValue *newNode = (struct KeyValue *)malloc(sizeof(struct KeyValue));
    newNode->key = strdup(key);
    newNode->value = value;
    newNode->next = map->table[index];
    map->table[index] = newNode;

    // Update LRU list
    if (map->lru_head == NULL)
    {
        map->lru_head = newNode;
        map->lru_tail = newNode;
    }
    else
    {
        newNode->prev = map->lru_tail;
        map->lru_tail->next = newNode;
        map->lru_tail = newNode;
    }

    // Check and remove the least recently used item if the cache exceeds the capacity
    if (map->size >= 20)
    {
        // Remove the least recently used item from the cache
        struct KeyValue *lruItem = map->lru_head;

        // Update the LRU head
        map->lru_head = lruItem->next;
        if (map->lru_head != NULL)
        {
            map->lru_head->prev = NULL;
        }

        // Remove the item from the hash table
        int lruIndex = hash(lruItem->key);
        struct KeyValue *current = map->table[lruIndex];
        struct KeyValue *prev = NULL;

        while (current != NULL)
        {
            if (strcmp(current->key, lruItem->key) == 0)
            {
                if (prev == NULL)
                {
                    map->table[lruIndex] = current->next;
                }
                else
                {
                    prev->next = current->next;
                }
                free(lruItem->key);
                free(lruItem);
                break;
            }
            prev = current;
            current = current->next;
        }
    }
    else
    {
        // Increment the cache size
        map->size++;
    }
}

// Search for a key in the hashmap and return its value
int get(struct LRU_HashMap *map, char *key)
{
    int index = hash(key);
    struct KeyValue *current = map->table[index];
    while (current != NULL)
    {
        if (strcmp(current->key, key) == 0)
        {
            // Update LRU list (move to the tail)
            if (current != map->lru_tail)
            {
                if (current == map->lru_head)
                {
                    map->lru_head = current->next;
                    map->lru_head->prev = NULL;
                }
                else
                {
                    current->prev->next = current->next;
                    current->next->prev = current->prev;
                }
                current->prev = map->lru_tail;
                map->lru_tail->next = current;
                map->lru_tail = current;
                current->next = NULL;
            }
            return current->value;
        }
        current = current->next;
    }
    return -1; // Key not found
}





// Data structure for hash based storage
struct HashMap
{
    char *file_path;
    struct StorageServerInfo *storage_server;
    struct HashMap *next;
};

// Function to create a new HashMap entry
struct HashMap *createHashMapEntry(const char *file_path, struct StorageServerInfo *ss)
{
    struct HashMap *entry = (struct HashMap *)malloc(sizeof(struct HashMap));
    entry->file_path = strdup(file_path);
    entry->storage_server = ss;
    entry->next = NULL;
    return entry;
}

// Function to perform a fast lookup in the HashMap
struct StorageServerInfo *efficientStorageServerSearch(struct HashMap *hashmap, const char *file_path)
{
    struct HashMap *current = hashmap;
    while (current != NULL)
    {
        if (strcmp(current->file_path, file_path) == 0)
        {
            return current->storage_server;
        }
        current = current->next;
    }
    return NULL; // File path not found in the HashMap
}




//////////////////////////////// MAIN FUNCTION ////////////////////////
int main(int argc, char* argv[])
{
    // Signal handler for Ctrl+C and Ctrl+Z
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    // Initialize server and client sockets
    clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    storageServerSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket < 0 || storageServerSocket < 0){
        perror("[-] Error in initializing socket");
        exit(EXIT_FAILURE);
    }

    // Initializing address for clientServer and Storage server
    struct sockaddr_in clientServerAddr, storageServerAddr;
    bzero(&clientServerAddr, sizeof(clientServerAddr));
    bzero(&storageServerAddr, sizeof(storageServerAddr));
    
    clientServerAddr.sin_family = AF_INET;
    clientServerAddr.sin_port = htons(CLIENT_PORT);
    clientServerAddr.sin_addr.s_addr = INADDR_ANY;

    storageServerAddr.sin_family = AF_INET;
    storageServerAddr.sin_port = htons(STORAGE_SERVER_PORT);
    storageServerAddr.sin_addr.s_addr = INADDR_ANY;

    // Binding storage server and client addresses
    if (bind(clientSocket, (struct sockaddr *)&clientServerAddr, sizeof(clientServerAddr)) < 0 ||
        bind(storageServerSocket, (struct sockaddr *)&storageServerAddr, sizeof(storageServerAddr)) < 0)
    {
        perror("[-] Error in binding sockets");
        closeConnections();
        exit(EXIT_FAILURE);
    }

    if (listen(clientSocket, NO_CLIENTS_TO_LISTEN_TO) < 0 || listen(storageServerSocket, NO_SERVER_TO_LISTEN_TO) < 0){
        perror("[-] Error in listening");
        closeConnections();
        exit(EXIT_FAILURE);
    }

    else{
        printf("[+] Nameserver initialized.\n");
        printf("[+] Listening for storage server initialization and client requests on IP address 127.0.0.1 ...\n");
    }

    // Allocating threads for direct communication with Storage Server and client
    storageServerThreads = (pthread_t*)malloc(sizeof(pthread_t) * MAX_SERVERS);
    clientThreads = (pthread_t*)malloc(sizeof(pthread_t) * MAX_CLIENTS);
    
    pthread_t clientThread, storageServerThread;
    pthread_create(&clientThread, NULL, (void*)handleClients, NULL);
    pthread_create(&storageServerThread, NULL, (void*)handleStorageServerInitialization, NULL);
    
    // The above function loops for ever so this end of code is never reached
    pthread_join(storageServerThread, NULL);
    pthread_join(clientThread, NULL);

    // Wait for user input to close connections
    printf("Press Enter to close server connections...\n");
    getchar();
    
    // Close server and client listening sokcet
    closeConnections();
    return 0;
}