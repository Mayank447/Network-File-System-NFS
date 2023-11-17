#ifndef HELPER_FUNCTIONS
#define HELPER_FUNCTIONS

// Remove leading whitespaces form a string
void trim(char *str); 

// Extract filename from path
void extractFileName(char *path, char *filename); 

// Download a File - Receive data from a Socket and write to a File
void downloadFile(char* filename, int socket); 

// Upload a File - Read a File and send data to a Socket
void uploadFile(char* filename, int socket); 

// print the error based on the valid bit to message string
void handleErrorCodes(int valid, char* message);

#endif