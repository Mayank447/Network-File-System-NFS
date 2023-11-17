#include "header_files.h"
#include "params.h"

// Remove leading whitespaces
void trim(char *str) {
    int i=0, j=0, len = strlen(str);
    while(isspace((unsigned char)str[i]) && i<len) i++;
    while(i<len && !isspace((unsigned char)str[i])) str[j++] = str[i++];
    str[j] = '\0';
}

// Extract filename from path
void extractFileName(char *path, char *filename) {
    const char *lastSlash = strrchr(path, '/');

    if (lastSlash != NULL) {
        strcpy(filename, lastSlash + 1);
    } else {
        strcpy(filename, path);
    }
}

// Download a File - Receive data from a Socket and write to a File
void downloadFile(int socket, char* filename){
    FILE* file = fopen(filename, "w");
    if(!file){
        printf("Unable to open the FILE %s for writing\n", filename);
        close(socket);
        return;
    }
    int bytesReceived;
    char buffer[BUFFER_LENGTH];
    while(1)
    {
        bytesReceived = recv(socket, buffer, sizeof(buffer), 0);
        if(bytesReceived == 0) break;
        if(bytesReceived < 0){
            perror("Error downloadFile(): Unable to receive the file data from the server");
            break;
        }

        printf("%s", buffer);
        if(fprintf(file, "%s", buffer) < 0){
            printf("Error downloadFile(): Unable to write to the file");
            break;
        }
    }
    fclose(file);
}
