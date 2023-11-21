#ifndef FILE_H
#define FILE_H

#ifndef STORAGE_SERVER_H
#include "storage_server.h"
#endif

// FILE STRUCT FUNCTIONS
int checkFilePathExists(char* path);
int validateFilePath(char* filepath, char* operation_no);
int addFile(char* path, int check);
int removeFile(char* path);
void decreaseReaderCount(char* path);
void openWriteLock(char* path);
void cleanUpFileStruct();

// ACTUAL FILE FUNCTIONS
int checkFileType(char* path);
int fileExists(char *filename);
void createFile(char* path, char* response);
void deleteFile(char *filename, char* response);
void getFileMetaData(char* filepath, int clientSocketID);
void copyFile(char* ss_path, char* response);
void copyDirectory(char* ss_path, char* response);
void DownloadFile(int serverSocket, char* filename);
void UploadFile(int serverSocket, char* filename);

#endif