# READ ME!
# NFS: ~~ Need For Speed ~~ Network File System Implementation

## ASSUMPTIONS
Woooooooooooooooooooooooooohooooooooohooooooooo

## Brief Overview of the various roles played in this drama named "NFS"

### NAME SERVER

This is the brain of the project, facilitating communication between clients and the storage servers. 
On requesting a file or folder by the clients, also provides with information about the specific storage server.
Basically a directory service while ensuring efficient and accurate file access.

Important Features to note:

#### LRU Implementation

Implemented LRU caching using doubly linked lists.
Functions:
- `Node UPDATEtoFront(Node node)`: Moves recently accessed storage server to the beginning of list
- `Node Search(char* key)`: Searches the storage servers accessible paths  for identification
- `Node ADD(SSInfo ssinfo)`: If not found in cache, add it to it.
- `void printCache()`: helper function to print the then contents of the cache, useful in debugging :)

#### Search Implementation

Implemented Search using HashMaps. On basis of the path length, unique hashing was done and path was linked to the storage server.
This enabled quick and efficient access for client requests.
Functions:
- `void deletePathFromHashTable(struct HashTable* hashTable, char* path)`: removes paths from the has table
- `void insertIntoHashTable(struct HashTable* hashTable, char* path, struct StorageServerInfo* ss_info)`: add new paths to hash table
- `struct StorageServerInfo* searchPathInHashTable(struct HashTable* hashTable, char* path)`: looks up th epath in the hash table.

####  Client Requests reg File/Folder

- **Writing:** Clients can actively create and update the content of files and folders within the NFS. This operation encompasses the storage and modification of data, ensuring that the NFS remains a dynamic repository.
- **Reading:** Reading operations empower clients to retrieve the contents of files stored within the NFS. This fundamental operation grants clients access to the information they seek.
- **Deleting:** Clients retain the ability to remove files and folders from the network file system when they are no longer needed, contributing to efficient space management.
- **Creating:** The NFS allows clients to generate new files and folders, facilitating the expansion and organization of the file system. This operation involves the allocation of storage space and the initialization of metadata for the newly created entities.
- **Getting File Meta Data:** Clients can access a wealth of supplementary information about specific files. This includes details such as file size, access rights, timestamps, and other metadata, providing clients with comprehensive insights into the files they interact with.

#### Features
- Handle multiple clients concurrently. 
- Set of well defined error codes that can be returned when a client’s request cannot be accommodated.
- Efficient Search using hashmaps to enhance response times, useful for systems with a large number of files and folders.
- LRU Caching using doubly linked lists to further improving response times and system efficiency.
- Failure Detection to promptly respond to any disruptions in SS availability.
- A logging mechanism to record and display every request or acknowledgment received from clients or Storage Servers.
- Log should has information such as IP addresses and ports used in each communication.

### Storage Servers

- Maintain information for multiple storage servers.
- Initialization of storage servers also happens here.

### CLIENT

The client communicates with a naming server to obtain the IP address and port of the relevant storage server based on the provided file path. It then establishes a connection with the storage server to perform file and directory operations. Nothing else -_-.

### HELPER_FUNCTIONS

These functions provide essential utility and file-handling tasks for the client-side code interacting with a storage system. They handle string manipulation, file name extraction, error code translation, and file transfer operations.

