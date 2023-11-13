#include "header_files.h"
#include "name_server.h"
#include "params.h"
#include "helper_functions.h"

int clientSocket, storageServerSocket; // Sockets for handling storage server queries and client requests
pthread_t* storageServerThreads; //Thread for direct communication with a Storage server
pthread_t* clientThreads; //Thread for direct communication with a client

// Arguments to send while creating a Storage server thread
struct storageServerArg{
    int socket;
    int ss_id;
};

void closeConnections(){ // Function to close server and client connection sockets
    close(clientSocket);
    close(storageServerSocket);
}

/* Signal handler in case Ctrl-Z or Ctrl-D is pressed -> so that the socket gets closed */
void handle_signal(int signum) {
    closeConnections();
    exit(signum);
}

///////////////////////// STORAGE SERVER INITIALIZATION///////////////////////////

int ss_count = 0; // Count of no. of storage servers
pthread_mutex_t ss_count_lock = PTHREAD_MUTEX_INITIALIZER;

struct StorageServerInfo *storageServerList = NULL; // Head of linked list(reversed) of storage server
// TODO - Need to implement a lock for storageServerHead and each storageServer

/* Function to add storage server (meta) information to the linked list */
struct StorageServerInfo* addStorageServerInfo(const char *ip, int ns_port, int cs_port)
{
    struct StorageServerInfo *newServer = (struct StorageServerInfo *)malloc(sizeof(struct StorageServerInfo));
    strcpy(newServer->ip_address, ip);
    newServer->naming_server_port = ns_port;
    newServer->client_server_port = cs_port;
    newServer->ss_id = ss_count;
    newServer->next = storageServerList; //Reversed linked list 
    for (int i = 0; i < MAX_NO_ACCESSIBLE_PATHS; i++)
    {
        newServer->accessible_paths[i][0] = '\0';
    }
    storageServerList = newServer;
    return newServer;
}


/* Thread handler for storage server request/queries (General/open handler) */
void* handleStorageServerInitialization()
{   
    struct storageServerArg args[MAX_SERVERS];

    while(1){
        struct sockaddr_in server_address;
        socklen_t address_size = sizeof(struct sockaddr_in);
        int serverSocket = accept(storageServerSocket, (struct sockaddr*)&server_address, &address_size);
        if (serverSocket < 0) {
            perror("Accept failed");
            continue;
        }

        printf("Connection accepted from %s:%d\n", inet_ntoa(server_address.sin_addr), ntohs(server_address.sin_port));
        
        // Create a new thread for each storage server
        args[ss_count].socket = serverSocket;
        args[ss_count].ss_id = ss_count + 1;

        pthread_mutex_lock(&ss_count_lock);
        if (pthread_create(&storageServerThreads[ss_count], NULL, (void*)handleStorageServer, (void*)&args[ss_count]) < 0) {
            perror("Thread creation failed");
            pthread_mutex_unlock(&ss_count_lock);
            continue;
        }

        // Detach the thread - resources of the thread can be released when it finishes without waiting for it explicitly.
        if (pthread_detach(storageServerThreads[ss_count]) != 0) {
            perror("Error detaching Storage server thread");
            continue;
        }

        ss_count++;
        pthread_mutex_unlock(&ss_count_lock);
    }
    return NULL;
}


/* Individual thread handler for a server*/
void* handleStorageServer(void* argument)
{
    struct storageServerArg* args = (struct storageServerArg*)argument;
    int connectedServerSocketID = args->socket;
    int serverID = args->ss_id;
    struct StorageServerInfo* server = NULL;

    int naming_server_port, client_server_port;
    char ip_address[256], buffer[BUFFER_LENGTH];
    int receivingInfo = 0; // State variable
    int pathIndex = -2;
    
    // Initialization process
    int bytesReceived = recv(connectedServerSocketID, buffer, BUFFER_LENGTH, 0);
    if (bytesReceived < 0){
        perror("Error in Initializing the server");
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
            
            if (send(connectedServerSocketID, &ss_id_str, sizeof(int), 0) < 0){
                perror("Error sending ss_id");
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
            pathIndex = 0;
        }

        // Receiving accessible paths
        else if (receivingInfo == 2 && pathIndex >= 0 && pathIndex < MAX_NO_ACCESSIBLE_PATHS){
            strcpy(server->accessible_paths[pathIndex++], token);
            printf("naming server storage:%s\n", token);
        }

        token = strtok(NULL, ":");
    }
    close(connectedServerSocketID);


    // Initializing the connection to specified Storage Server's PORT number
    int serverSocket = initConnectionToStorageServer(server);
    if(serverSocket != -1){
        server->serverSocket = serverSocket;
        printf("Storage server %d initialization done.\n", serverID);
    }
    else{
        printf("Error connecting to specificied PORT. This error needs to be handled.");
    }
    connectedServerSocketID = server->serverSocket;


    // Handling requests from storage server.  
    bzero(buffer, BUFFER_LENGTH);
    while(1){
        if((bytesReceived = recv(connectedServerSocketID, buffer, sizeof(buffer), 0)) < 0){
            printf("Storage server %d is down", serverID);
            break;
        }
    }
    return NULL;
}


/* Function to initialize the connection to storage servers with the specified PORT number*/
int initConnectionToStorageServer(struct StorageServerInfo* server)
{
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if(serverSocket == -1) {
        perror("Error in opening a socket");
        return -1;
    }

    struct sockaddr_in storageServerAddr;
    storageServerAddr.sin_family = AF_INET;
    storageServerAddr.sin_port = htons(server->naming_server_port);
    storageServerAddr.sin_addr.s_addr = INADDR_ANY;
    if (inet_pton(AF_INET, "127.0.0.1", &storageServerAddr.sin_addr.s_addr) < 0){
        perror("Invalid address/Address not supported");
        close(serverSocket);
        return -1;
    }

    if (connect(serverSocket, (struct sockaddr *)&storageServerAddr, sizeof(storageServerAddr)) < 0){
        perror("Error in connecting to the storage server");
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
        fprintf(stderr, "Error parsing storage server info: %s\n", data);
    }
}


/////////////////////////// FUNCTION TO HANDLE CLIENTS REQUESTS/QUERIES //////////////////////////
void handleClients()
{
    while(1){
        struct sockaddr_in client_address;
        socklen_t address_size = sizeof(struct sockaddr_in);
        int newClientSocket = accept(clientSocket, (struct sockaddr*)&client_address, &address_size);
        if (newClientSocket < 0) {
            perror("Error handleClients(): Client accept failed");
            continue;
        }
        
        printf("Client connection request accepted from %s:%d\n", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));
        
        // Create a new thread for each new client
        pthread_t clientThread;
        if (pthread_create(&clientThread, NULL, (void*)handleClientRequests, (void*)&newClientSocket) < 0) {
            perror("Thread creation failed");
            continue;
        }

        // Detach the thread
        if (pthread_detach(clientThread) != 0) {
            perror("Error detaching client thread");
            continue;
        }
    }
    return;
}

/* Function to handle a client request */
void handleClientRequests(int clientSocket){
    int var_recv = 0;
    char buffer[BUFFER_LENGTH];
    int bytesReceived = recv(clientSocket, &var_recv, sizeof(var_recv), 0);
    if (bytesReceived < 0) {
        perror("Error in receiving data");
        exit(1);
    }

    if(var_recv == 1){
        // code to receive file_path from client
        bytesReceived = recv(clientSocket, buffer, BUFFER_LENGTH, 0);
        if (bytesReceived < 0) {
            perror("Error in receiving data");
            exit(1);
        }
        buffer[bytesReceived] = '\0';
        // printf("Requested file_path: %s\n", buffer); // debugging
        //printf("hai 1 %s\n",buffer);
        // Check if the file exists in the storage servers
        struct StorageServerInfo* storageServer = searchStorageServer(buffer);
        if (storageServer != NULL) {
            // File found in storage server, send its details to the client
            //printf("not null %s\n",buffer);
            char response[1024];
            snprintf(response, sizeof(response), "%s:%d", storageServer->ip_address, storageServer->client_server_port);
            send(clientSocket, response, strlen(response), 0);
        } else {
            // File not found, send a response to the client
            char response[1024] = "FILE NOT FOUND";
            send(clientSocket, response, strlen(response), 0);
        }
    }
}


/////////////////////////// FUNCTIONS TO HANDLE STORAGE SERVER QUERYING /////////////////////////



// Function to search for a path in storage servers
struct StorageServerInfo* searchStorageServer(char* file_path) {
    int found = 0;
    struct StorageServerInfo* temp = storageServerList;
    while (temp != NULL) {
        // Search accessible paths in the current storage server
        int i = 0;
        while (temp->accessible_paths[i][0] != '\0') {  // Check if the path is not an empty string
            //printf("%s %s\n",file_path,temp->accessible_paths[i]);
            if (strncmp(file_path, temp->accessible_paths[i],strlen(file_path)) == 0) {
                found = 1;
                break; // Path found, no need to continue searching
            }
            i++;
        }
        if(found){
            break;
        }
        temp = temp->next;
    }

    if (found) {
        printf("hai 1 path found\n");
        return temp; // Found the path in a storage server
    } else {
        printf("hai path found\n");
        return NULL; // Path not found in any storage server
    }
}



// // Initialize a client
// void initializeClient(const char* nameServerIP, int nameServerPort) {
//     // Create a socket for communication with the naming server
//     int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
//     if (clientSocket < 0) {
//         perror("Error: opening client socket\n");
//         exit(1);
//     }
//     // Create a sockaddr_in structure for the naming server
//     struct sockaddr_in nameServerAddr;
//     memset(&nameServerAddr, 0, sizeof(nameServerAddr));
//     nameServerAddr.sin_family = AF_INET;
//     nameServerAddr.sin_port = htons(nameServerPort);
//     if (inet_pton(AF_INET, nameServerIP, &nameServerAddr.sin_addr) <= 0) {
//         perror("Error: invalid name server IP address\n");
//         exit(1);
//     }
//     // Connect to the naming server
//     if (connect(clientSocket, (struct sockaddr*)&nameServerAddr, sizeof(nameServerAddr)) < 0) {
//         perror("Error: connecting to the naming server\n");
//         exit(1);
//     }
//     // Construct a message with vital details for the current client
//     char clientIP[16];
//     int clientPort;
//     // Extract client details (e.g., from user input)
//     // For example:
//     // sscanf(clientAddress, "%[^:]:%d", clientIP, &clientPort);
//     char vitalDetailsMessage[1024];
//     snprintf(vitalDetailsMessage, sizeof(vitalDetailsMessage), "IP: %s\nPORT: %d\n", clientIP, clientPort);
//     // Send vital details to the naming server
//     if (send(clientSocket, vitalDetailsMessage, strlen(vitalDetailsMessage), 0) < 0) {
//         perror("Error: sending vital details to naming server\n");
//         exit(1);
//     }
//     // Close the socket when done
//     close(clientSocket);
// }




// Mutex for protecting the fileTable
pthread_mutex_t fileTableMutex = PTHREAD_MUTEX_INITIALIZER;
// Mutex for controlling server initialization
pthread_mutex_t serverInitMutex = PTHREAD_MUTEX_INITIALIZER;
// Sockets for clients and storage servers

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
        perror("Error in initializing socket");
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
        perror("Error in binding sockets");
        closeConnections();
        exit(EXIT_FAILURE);
    }

    if (listen(clientSocket, NO_SERVER_TO_LISTEN_TO) < 0 || listen(storageServerSocket, NO_CLIENTS_TO_LISTEN_TO) < 0){
        perror("Error in listening");
        closeConnections();
        exit(EXIT_FAILURE);
    }

    else{
        printf("Nameserver initialized.\n");
        printf("Listening for storage server initialization and client requests on IP address 127.0.0.1 ...\n");
    }

    // Allocating threads for direct communication with Storage Server and client
    storageServerThreads = (pthread_t*)malloc(sizeof(pthread_t) * MAX_SERVERS);
    clientThreads = (pthread_t*)malloc(sizeof(pthread_t) * MAX_CLIENTS);
    
    pthread_t clientThread, storageServerThread;
    pthread_create(&clientThread, NULL, (void*)handleClients, NULL);
    pthread_create(&storageServerThread, NULL, (void*)handleStorageServerInitialization, NULL);
    
    // The above function loops for ever so this end of code is never reached
    pthread_join(clientThread, NULL);
    pthread_join(storageServerThread, NULL);

    // Wait for user input to close connections
    printf("Press Enter to close server connections...\n");
    getchar();
    
    // Close server and client listening sokcet
    closeConnections();
    return 0;
}