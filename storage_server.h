#ifndef SERVER_H
#define SERVER_H

// Functions for files
void createFile(char* filename, int clientSocketID);
void uploadFile(char* filename, int clientSocketID);
void getFile(char* filename, int clientSocketID);
void deleteFile(char* filename, int clientSocketID);
void getFileAdditionalInfo(char* filename, int clientSocketID);

// Functions for director
void createDir(char* directoryname, int clientSocketID);
void uploadDir(char* directoryname, int clientSocketID);
void deleteDir(char* directoryname, int clientSocketID);
void getDir(char* directoryname, int clientSocketID);
void listFilesDir(char* directoryname, int clientSocketID);

#endif