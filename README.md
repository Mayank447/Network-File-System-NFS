# NAME SERVER

<!-- ## Overview

This Nameserver implementation manages communication between storage servers and clients. It handles storage server initialization, client requests, and storage server querying. -->

## LRU Implementation
Implemented LRU caching using doubly linked lists.
Functions:
- `Node UPDATEtoFront(Node node)`: Moves recently accessed storage server to the beginning of list
- `Node Search(char* key)`: Searches the storage servers accessible paths  for identification
- `Node ADD(SSInfo ssinfo)`: If not found in cache, add it to it.
- `void printCache()`: helper function to print the then contents of the cache, useful in debugging :)

## Global Variables

- `int clientSocket`: Socket for handling client requests.
- `int storageServerSocket`: Socket for handling storage server queries.
- `pthread_t* storageServerThreads`: Thread for direct communication with a Storage server.
- `pthread_t* clientThreads`: Thread for direct communication with a client.
- `pthread_mutex_t ss_count_lock`: Mutex for the storage server count.
- `struct storageServerArg`: Struct for passing arguments to the storage server thread.
  - `int socket`: Socket for a storage server.
  - `int ss_id`: Storage server ID.

- `int ss_count`: Count of the number of storage servers.
- `struct StorageServerInfo *storageServerList`: Head of the linked list of storage servers.
  - `char ip_address[256]`: IP address of the storage server.
  - `int naming_server_port`: Port for communication with the naming server.
  - `int client_server_port`: Port for communication with the client.
  - `int ss_id`: Storage server ID.
  - `char accessible_paths[MAX_NO_ACCESSIBLE_PATHS][256]`: Array of accessible paths.
  - `struct StorageServerInfo *next`: Pointer to the next storage server in the linked list.
  - `struct StorageServerInfo *redundantSS_1`: Pointer to the first redundant storage server.
  - `struct StorageServerInfo *redundantSS_2`: Pointer to the second redundant storage server.


## Functions

### `closeConnections()`

#### Signature
```c
void closeConnections();
```

#### Input

No explicit input.

#### Task Performed

Closes the client and storage server connection sockets.

#### Output

No explicit output.

### `handle_signal(int signum)`

#### Signature

```c
void handle_signal(int signum);
```

#### Input

- `signum`: Signal number indicating the type of signal received.

#### Task Performed

Handles signals such as Ctrl+C and Ctrl+Z by closing connections and terminating the program.

#### Output

No explicit output.

### `addStorageServerInfo(const char *ip, int ns_port, int cs_port)`

#### Signature

```c
struct StorageServerInfo* addStorageServerInfo(const char *ip, int ns_port, int cs_port);
```

#### Input

- `ip`: IP address of the storage server.
- `ns_port`: Port for communication with the naming server.
- `cs_port`: Port for communication with clients.

#### Task Performed

Creates a new `StorageServerInfo` struct, initializes its fields, and adds it to the linked list.

#### Output

Returns a pointer to the newly created `StorageServerInfo` struct.

### `handleStorageServerInitialization()`

#### Signature

```c
void* handleStorageServerInitialization();
```

#### Input

No explicit input.

#### Task Performed

Handles initialization of storage servers by accepting connections and creating threads for each new created storage server and increses the global count.

#### Output

No explicit output.

### `handleStorageServer(void* argument)`

#### Signature

```c
void* handleStorageServer(void* argument);
```

#### Input

- `argument`: Pointer to a `struct storageServerArg` containing socket and storage server ID.

#### Task Performed

Handles storage server requests, initializes the connection to a specified storage server's port, and handles server communication by receiving it's vital informations ans stores them in the node related to storage server and sends the number  to storage server on successful storage of information.

#### Output

No explicit output.

### `initConnectionToStorageServer(struct StorageServerInfo* server)`

#### Signature

```c
int initConnectionToStorageServer(struct StorageServerInfo* server);
```

#### Input

- `server`: Pointer to a `StorageServerInfo` struct containing server information.

#### Task Performed

Initializes a TCP connection to the specified storage server's port.

#### Output

Returns the connected socket or -1 on failure.

### `parseStorageServerInfo(const char *data, char *ip_address, int *ns_port, int *cs_port)`

#### Signature

```c
void parseStorageServerInfo(const char *data, char *ip_address, int *ns_port, int *cs_port);
```

#### Input

- `data`: String containing storage server information.
- `ip_address`: Pointer to a buffer for storing IP address.
- `ns_port`: Pointer to an integer for storing naming server port.
- `cs_port`: Pointer to an integer for storing client server port.

#### Task Performed

Parses storage server information and extracts IP address, naming server port, and client server port and stores them in the required variables.

#### Output

No explicit output.

### `handleClients()`

#### Signature

```c
void* handleClients();
```

#### Input

No explicit input.

#### Task Performed

Handles new client requests by accepting connections and creating threads for each client.

#### Output

No explicit output.

### `handleClientRequests(void* socket)`

#### Signature

```c
void* handleClientRequests(void* socket);
```

#### Input

- `socket`: Pointer to an integer representing the client socket.

#### Task Performed

Handles client requests, searches for the storage server that stores a specific path, and sends the storage server's IP and port back to the client or the error code on failure.

#### Output

No explicit output.

### `searchStorageServer(char* file_path)`

#### Signature

```c
struct StorageServerInfo* searchStorageServer(char* file_path);
```

#### Input

- `file_path`: Path to be searched in storage servers.

#### Task Performed

Searches for a path in storage servers using a linear search.

#### Output

Returns a pointer to the `StorageServerInfo` struct if the path is found; otherwise, returns NULL.


### `main(int argc, char* argv[])`

#### Signature

```c
int main(int argc, char* argv[]);
```

#### Input

- `argc`: Number of command-line arguments.
- `argv`: Array of command-line argument strings.

#### Task Performed

Main function initializing the server and client sockets, binding addresses, and creating threads for client and storage server communication.

#### Output

Returns 0 upon successful execution.
and modification.

# STORAGE SERVER    

## Global Variables:

- `int ss_id`: Storage server ID
- `int clientPort`: PORT for communication with the client
- `int clientSocketID`: Main binded socket for accepting requests from clients
- `int nsSocketID`: Main socket for communication with the Name server
- `int nsPort`: PORT for communication with Name server (user-specified)
- `char nsIP[16]`: Assuming IPv4
- `File* fileHead`: Global pointer for the beginning of the file in the struct linked list
- `File* fileTail`: Global pointer for the last file in the struct linked list

###  `closeConnection()`

#### signatue 
```c
void closeConnection()
```

#### Input:
No input is taken.

#### Task:
Closes the sockets and cleans the data structures associated.


---

### `handle_signal(int signum)`

#### signatue 

```c
void handle_signal(int signum)
```

#### Input:
It takes the signal number as input.

#### Task:
It calls the `closeConnection` function to clean up the data structures, close sockets, and exits the program.

---

### `checkFilePathExists(char* path)`
#### signatue 

```c
int checkFilePathExists(char* path)
```

#### Input:
String containing the path.

#### Task:
Searches for all the paths in the file structure by comparing.
Returns 1 if found, else returns 0.

---

## File Structure Functions:

### c`heckFileType(char* path)`

#### signatue 

```c
int checkFileType(char* path)
```
#### Input:
String containing the path.

#### Task:
Check the existence of a path and whether it corresponds to a file/directory.it returns 0 if it is file , 1 if it is directory else -1 it is not found


### `void cleanUpFileStruct()`

#### signatue 

```c
void cleanUpFileStruct()
```

#### Input:
No input is taken.

#### Task:
Cleans up the file structure by freeing allocated memory.

### `decreaseReaderCount(File* file)`

#### signatue 

```c
void decreaseReaderCount(File* file)
```

#### Input:
Pointer to a file in the struct.

#### Task:
Decreases the reader count of a particular file.It locks the critical section and makes read_write as -1 and unlocks when the reader count is 0.

### `closeWriteLock(File* file)`

#### signatue 

```c
void closeWriteLock(File* file)
```

#### Input:
Pointer to a file in the struct.

#### Task:
Frees the file lock from writing.
 by updating the read_write variable of the file struct.

### `addFile(char* path, int check)`

#### signatue 

```c
int addFile(char* path, int check)
```

#### Input:
- `char* path`: Path to be added.
- `int check`: Check if the path is not already stored in the storage server.

#### Task:
Adds a new file to the file struct if the path is not already stored.It checks if the path already exists if not then creates it and adds to file struct and initialises all the variables and initialises locks and adds to the file struct

#### Output:
Returns 0 on success, specific error codes on failure.

### `removeFile(char* path)`

#### signatue 

```c
int removeFile(char* path)
```

#### Input:
String containing the path.

#### Task:
It searches for the file ,if found Removes a file from the file struct.

#### Output:
Returns 1 if removed, 0 if not found.

---

## Functions for Initializing the Connection with the Name Server:

### `sendInfoToNamingServer(const char *nsIP, int nsPort, int clientPort)`

#### signatue 

```c
int sendInfoToNamingServer(const char *nsIP, int nsPort, int clientPort)
```

#### Input:
- `const char* nsIP`: IP address of the Naming Server.
- `int nsPort`: PORT for communication with the Naming Server.
- `int clientPort`: PORT for communication with the client.

#### Task:
Sends vital information to the Naming Server and receives the ss_id.On initialisation,the storage server sends its IPAddress ,its name server port ,client port to the name server using TCP connection and sends paths accessable and at last it sends the completed message to indicate the completion of the transmission.It also opens the sockets with the client port and naming server port for communication.It receives the number as acknowledgement on suceessful transmission of information to the naming server.

#### Output:
Returns 0 on success, -1 on failure.

### `int open_a_connection_port(int Port, int num_listener)`

#### signatue 

```c
int open_a_connection_port(int Port, int num_listener)
```

#### Input:
- `int Port`: PORT to open the connection.
- `int num_listener`: Number of listeners.

#### Task:
It opens a connection by creating sockets , binding them and listening to the max listeners.

#### Output:
Returns the opened socket.

### `collectAccessiblePaths()`

#### signatue 

```c
void collectAccessiblePaths()
```

#### Task:
It Collects accessible paths from the user and checks if they actually exists ,if not return error of non-existance if present, stores them in a file and also in file structure calling addFile function.

---

## Functions to Handle Key Communication with the Name Server:

---

## Functions to Handle Communication with the Client:

### `receiveAndValidateRequestNo(int clientSocket)`

#### signatue 

```c
int receiveAndValidateRequestNo(int clientSocket)
```

#### Input:
- `int clientSocket`: Socket for communication with the client.

#### Task:
Receives and validates the request number from the client and sends error to the client if it is not valid.

#### Output:
Returns the request number.

### `validateFilePath(char* filepath, int operation_no, File* file)`

#### signatue 

```c
int validateFilePath(char* filepath, int operation_no, File* file)
```

#### Input:
- `char* filepath`: Path to be validated.
- `int operation_no`: Operation number.
- `File* file`: Pointer to a file in the struct.

#### Task:
Validates the filepath based on the operation number and it also locks the files critical section according to the operation to prevent overwriting and also changes the variable associated with the operation.

#### Output:
Returns 0 on success, specific error codes on failure.

### `receiveAndValidateFilePath(int clientSocket, char* filepath, int operation_no, File* file, int validate)`

#### signatue 

```c
int receiveAndValidateFilePath(int clientSocket, char* filepath, int operation_no, File* file, int validate)
```

#### Input:
- `int clientSocket`: Socket for communication with the client.
- `char* filepath`: Path to be received and validated.
- `int operation_no`: Operation number.
- `File* file`: Pointer to a file in the struct.
- `int validate`: Flag for validation.

#### Task:
Receives the filepath from client and validates the filepath based on the operation number by giving them to validateFilePath function and sends errors to the client.

#### Output:
Returns the validation status.

### `handleClients()`

#### signatue 

```c
void handleClients()
```

#### Task:
It loops foreever by trying to accept the incomming client connections and handles incoming client requests by creating a new thread for each client.

### `handleClientRequest(void* argument)`

#### signatue 

```c
void* handleClientRequest(void* argument)
```

#### Input:
- `void* argument`: Argument passed to the thread.

#### Task:
Handles client requests based on the received request number ,it validates then ,calls the functions to perform the task related to the request number sent by the client.

### `createFile(char* path, int clientSocket)`

#### signatue 

```c
void createFile(char* path, int clientSocket)
```

#### Input:
- `char* path`: Path of the file to be created.
- `int clientSocket`: Socket for communication with the client.

#### Task:
Creates a file and updates the file struct and sends the status of file creation , if fails sends the error codes corresponding to the error.

### `createDirectory(char* path, int clientSocket)`

#### signatue 

```c
void createDirectory(char* path, int clientSocket)
```

### `deleteFile(char* filename, int clientSocketID)`

#### signatue 

```c
void deleteFile(char* filename, int clientSocketID)
```

#### Input:
- `char* filename`: Name of the file to be deleted.
- `int clientSocketID`: Socket for communication with the client.

#### Task:
Checks for the permission, sends the status of deletion to the client ,Deletes a file and updates the file struct.

### `getFileMetaData(char* filepath, int clientSocketID)`

#### signatue 

```c
void getFileMetaData(char* filepath, int clientSocketID)
```

#### Input:
- `char* filepath`: Path of the file.
- `int clientSocketID`: Socket for communication with the client.

#### Task:
Gets file metadata by using the stat functionn and sends it to the client.If error sends the errors to clients along with error codes.


### `deleteDirectory(const char *path, int clientSocketID)`

#### signatue 

```c
void deleteDirectory(const char *path, int clientSocketID)
```

#### Input 
- `const char *path`: The path of the directory to be deleted.
- `int clientSocketID`: Socket identifier for communication with the client.

#### Task
The `deleteDirectory` function is designed to recursively delete a directory and its contents, handling both subdirectories and files. It communicates status messages back to the client through the specified socket ID.Checks if the provided path exists and is a directory. If not, sends an error message to the client and exits.Opens the directory and iterates through its entries using a while loop.Skips entries representing the current and parent directories (`.` and `..`).For subdirectories, makes a recursive call to `deleteDirectory` for their deletion.For files, calls a separate function `deleteFile` and attempts to remove them.Sends error messages to the client if there are issues deleting files or directories.Removes the empty directory after successfully deleting its contents.Sends status messages and error details to the client through the specified socket ID.

## Main Function:

### `main(int argc, char* argv[])`

#### signatue 

```c
int main(int argc, char* argv[])
```

#### Input:
- `int argc`: Number of command-line arguments.
- `char* argv[]`: Array of command-line arguments.

#### Task:
Main function to execute the storage server.It prompts to the user asking the Ip address , ports and accessable paths calls signal functions to handle the signals and calls all the functions to send information to naming server and opening connections for communication.

#### Output:
Returns 0 on success, specific error codes on failure.

---
# CLIENT


<!-- ## Overview

The client communicates with a naming server to obtain the IP address and port of the relevant storage server based on the provided file path. It then establishes a connection with the storage server to perform file and directory operations. -->

## Functions

### `printOperations()`

#### Signature

```c
void printOperations();
```

#### Input

No explicit input.

#### Task Performed

Prints a menu of possible file and directory operations for the user to choose from.

#### Output

No explicit output.

### `getFilePath(char* path1)`

#### Signature

```c
void getFilePath(char* path1);
```

#### Input

- `path1`: Buffer to store the entered file path.

#### Task Performed

Prompts the user to enter a file path and stores it in the provided buffer.

#### Output

No explicit output.

### `fetchStorageServerIP_Port(const char* path, char* IP_address, int* PORT)`

#### Signature

```c
int fetchStorageServerIP_Port(const char* path, char* IP_address, int* PORT);
```

#### Input

- `path`: File path for which storage server information is needed.
- `IP_address`: Buffer to store the IP address of the storage server.
- `PORT`: Pointer to an integer to store the port number of the storage server.

#### Task Performed

Establishes a connection with the naming server to fetch the storage server's IP address and port based on the provided file path.

#### Output

Returns 0 on success; -1 on failure.

### `parseIpPort(char *data, char *ip_address,int *ss_port)`

#### Signature

```c
int parseIpPort(char *data, char *ip_address,int *ss_port);
```

#### Input

- `data`: String containing storage server information.
- `ip_address`: Buffer to store the parsed IP address.
- `ss_port`: Pointer to an integer to store the parsed port number.

#### Task Performed

Parses the storage server information and extracts IP address and port.

#### Output

Returns 0 on success; -1 on failure.

### `connectToStorageServer(const char* IP_address, const int PORT)`

#### Signature

```c
int connectToStorageServer(const char* IP_address, const int PORT);
```

#### Input

- `IP_address`: IP address of the storage server.
- `PORT`: Port number of the storage server.

#### Task Performed

Connects to the storage server using the provided IP address and port to perform operation like read ,write ,getpermissions of file.

#### Output

Returns the connected socket on success; -1 on failure.

### `checkResponse(char* response)`

#### Signature

```c
int checkResponse(char* response);
```

#### Input

- `response`: String containing the server's response.

#### Task Performed

Checks the server's response for error codes and prints corresponding error messages.

#### Output

Returns 0 if the response is valid; -1 on error.

### `receiveAndCheckResponse(int serverSocket, char* error)`

#### Signature

```c
int receiveAndCheckResponse(int serverSocket, char* error);
```

#### Input

- `serverSocket`: Socket for communication with the storage server.
- `error`: Error message to print in case of failure.

#### Task Performed

Receives the server's response and checks it for error codes.

#### Output

Returns 0 if the response is valid; -1 on error.

### `sendOpNumber_Path(int serverSocket, char* operation_no_path)`

#### Signature

```c
int sendOpNumber_Path(int serverSocket, char* operation_no_path);
```

#### Input

- `serverSocket`: Socket for communication with the storage server.
- `operation_no_path`: String containing the operation number or file path to send.

#### Task Performed

Sends the operation number or file path to the connected storage server.

#### Output

Returns 0 on success; -1 on failure.

### `connectAndCheckForFileExistence(char* path, char* operation_num)`

#### Signature

```c
int connectAndCheckForFileExistence(char* path, char* operation_num);
```

#### Input

- `path`: File path to check for existence.
- `operation_num`: Operation number corresponding to the desired operation.

#### Task Performed

Connects to the storage server, checks if the file exists, and sends the operation number for searching file path and file path.

#### Output

Returns the connected socket on success; -1 on failure.

### `createFile(char* path)`

#### Signature

```c
void createFile(char* path);
```

#### Input

- `path`: File path for creating a new file.

#### Task Performed

Performs the operation to create a new file.

#### Output

No explicit output.

### `deleteFile(char* path)`

#### Signature

```c
void deleteFile(char* path);
```

#### Input

- `path`: File path for deleting a file.

#### Task Performed

Performs the operation to delete an existing file.

#### Output

No explicit output.

### `readFile(char* path)`

#### Signature

```c
void readFile(char* path);
```

#### Input

- `path`: File path for reading the file.

#### Task Performed

Performs the operation to read the contents of an existing file.

#### Output

No explicit output.

### `writeToFile(char* path, char* data)`

#### Signature

```c
void writeToFile(char* path, char* data);
```

#### Input

- `path`: File path for writing data.
- `data`: Data to be written to the file.

#### Task Performed

Performs the operation to write data to an existing file.

#### Output

No explicit output.

### `getPermissions(char* path)`

#### Signature

```c
void getPermissions(char* path);
```

#### Input

- `path`: File path to get permissions.

#### Task Performed

Performs the operation to retrieve file permissions .

#### Output

No explicit output.

### `parseMetadata(char *data)`

#### Signature

```c
void parseMetadata(char *data);
```

#### Input

- `data`: String containing metadata information.

#### Task Performed

Parses and prints file metadata.

#### Output

No explicit output.

### `main()`

#### Signature

```c
int main();
```

#### Input

No explicit input.

#### Task Performed

Main function to execute the client, taking user input and performing corresponding file and directory operations.

#### Output

Returns 0 on successful execution.

# HELPER_FUNCTIONS

<!-- ## Overview

These functions provide essential utility and file-handling tasks for the client-side code interacting with a storage system. They handle string manipulation, file name extraction, error code translation, and file transfer operations. -->

## Functions

### `trim(char *str)`

#### Signature

```c
void trim(char *str);
```

#### Input

- `str`: Pointer to a string to be trimmed.

#### Task Performed

Removes leading whitespaces from the provided string.

#### Output

No explicit output.

### `extractFileName(char *path, char *filename)`

#### Signature

```c
void extractFileName(char *path, char *filename);
```

#### Input

- `path`: Full file path.
- `filename`: Buffer to store the extracted file name.

#### Task Performed

Extracts the file name from the provided path.

#### Output

No explicit output.

### `handleErrorCodes(int valid, char* message)`

#### Signature

```c
void handleErrorCodes(int valid, char* message);
```

#### Input

- `valid`: Integer representing an error code.
- `message`: Buffer to store the corresponding error message.

#### Task Performed

Translates error codes into human-readable error messages.

#### Output

No explicit output.

### `downloadFile(char* filename, int socket)`

#### Signature

```c
void downloadFile(char* filename, int socket);
```

#### Input

- `filename`: Name of the file to be downloaded.
- `socket`: Socket for communication with the storage server.

#### Task Performed

Receives file data from the provided socket and writes it to a local file.

#### Output

No explicit output.

### `uploadFile(char *filename, int clientSocket)`

#### Signature

```c
void uploadFile(char *filename, int clientSocket);
```

#### Input

- `filename`: Name of the file to be uploaded.
- `clientSocket`: Socket for communication with the storage server.

#### Task Performed

Opens the specified file for reading, sends an acknowledgment message to the server, and then uploads the file data.

#### Output

No explicit output.