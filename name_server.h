#ifndef NAME_SERVER_H
#define NAME_SERVER_H

#include "params.h"

#define CACHE_SIZE 4

typedef struct node_info* Node;
typedef struct StorageServerInfo SSInfo;

struct node_info{
    Node Prev;
    Node Next;
    struct StorageServerInfo *SS; 
};

struct StorageServerInfo{
    int ss_id;
    int serverSocket;
    char ip_address[30];
    int naming_server_port;
    int client_server_port;

    char accessible_paths[MAX_NO_ACCESSIBLE_PATHS][MAX_LENGTH_OF_ACCESSIBLE_PATH];
    int count_accessible_paths;
    pthread_mutex_t count_accessible_path_lock;

    int running;
    struct StorageServerInfo* next;
    struct StorageServerInfo* redundantSS_1;
    struct StorageServerInfo* redundantSS_2;
};

struct StorageServerInfo* addStorageServerInfo(const char *ip, int ns_port, int cs_port);
struct StorageServerInfo* searchStorageServer(char* file_path);
void cleanStorageServerInfoLinkedList();

int initConnectionToStorageServer(struct StorageServerInfo* server);
void parseStorageServerInfo(const char *data, char *ip_address, int *ns_port, int *cs_port);
void* handleStorageServer(void* argument);

void* handleClientRequests(void*);
void createDeletionHandler(char* path, char* response, char* type);
void copyHandler(char* path1, char* path2, char* response, char* op);

Node UPDATEtoFront(Node node);
Node Search(char* key);
Node ADD(SSInfo ssinfo);
void printCache();

#endif