#ifndef HELPER_FUNCTIONS
#define HELPER_FUNCTIONS

// Remove leading whitespaces form a string
void trim(char *str); 

// Extract filename from path
void extractFileName(char *path, char *filename); 

// Download a File - Receive data from a Socket and write to a File
void downloadFile(int socket, char* filename); 

#endif