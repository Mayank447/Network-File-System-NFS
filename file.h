#ifndef FILE_H
#define FILE_H

// FILE STRUCT FUNCTIONS
int checkFilePathExists(char* path);
int addFile(char* path, int check);
int removeFile(char* path);
void decreaseReaderCount(File* file);
void openWriteLock(File* file);
void cleanUpFileStruct();

// ACTUAL FILE FUNCTIONS
int checkFileType(char* path);
void createFile(char* path, int clientSocket);
void deleteFile(char *filename, int clientSocketID);

#endif