#ifndef SERVER_H
#define SERVER_H

// Some Macros
#define MAX_FILENAME_LENGTH 255
#define MAX_DIRNAME_LENGTH 255
#define MAX_FILES 10  // To be converted to a linked list
#define MAX_SUBDIRECTORIES 10 // To be converted to a linked list
#define MAX_PATH_LENGTH 100
#define MAX_ACCESSIBLE_PATHS 100

// Error codes
#define ERROR_LIMIT_REACHED 1
#define ERROR_STORAGE_SERVER_NOT_FOUND 2
#define ERROR_IO 3
#define ERROR_SOCKET 4

typedef struct file_struct{
    char name[MAX_FILENAME_LENGTH]; // filename
    char path[MAX_PATH_LENGTH];     // filepath (wrt root directroy
    int size;                       // filesize in bytes
    int ownerID;                    // owner 
    int is_locked;                  // concurrency lock (0 for not locked, 1 for read lock, 2 for write lock)
    int write_client_id;            // clientID of the client modifting it
    int reader_count;               // no. of clients reading this file
    struct file_struct* nextfile;   // pointer to the next filenode in the directory
} File;

typedef struct dir_struct{
    char name[MAX_DIRNAME_LENGTH];    // directory name
    char path[MAX_PATH_LENGTH];       // directory path (wrt root directory)
    int file_count;                   // no. of files inside the directory (not subdirectories)
    File* file_head;                  // head of singly linked list of files
    FILE* file_tail;                  // tail of singly linked list of files
    
    int subdir_count;                 // no. of subdirectories (not sub-subdirectories)
    struct dir_struct *subdir_head;   // head of singly linked list of subdirectories
    struct dir_struct* subdir_tail;   // tail of singly linked list of subdirectories
    struct dir_struct* next_subDir;   // pointer to the next subdirectory

} Directory;

struct StorageServer {
    char IP_Address[16];        // IP Address of the Storage Server
    int NS_Port;                // Port for communication with Name serve
    int Client_Port;            // Port for communication with Client
    char accessible_paths[MAX_ACCESSIBLE_PATHS][MAX_PATH_LENGTH];
    Directory* root_directory;  // Root directory for the storage server
    pthread_mutex_t lock;       // Lock mechanism (needs to be modified)
};

// Functions for files
void createFile(Directory* parent, const char* filename, int ownerID);
void uploadFile(char* filename, int clientSocketID);
void getFile(char* filename, int clientSocketID);
void deleteFile(char* filename, int clientSocketID);
void getFileAdditionalInfo(char* filename, int clientSocketID);
int lockFile(File *file, int lock_type);
void write_releaseLock(File *file, int clientID);
void read_releaseLock(File *file);

// Functions for directory
void createDir(Directory* parent, const char* dirname);
void uploadDir(char* directoryname, int clientSocketID);
void deleteDir(char* directoryname, int clientSocketID);
void getDir(char* directoryname, int clientSocketID);
void listDirectoryContents(Directory* dir);

#endif