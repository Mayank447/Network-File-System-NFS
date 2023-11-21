#ifndef FILE_H
#define FILE_H

// FILE STRUCT FUNCTIONS
int checkFilePathExists(char* path);
int validateFilePath(char* filepath, int operation_no, File* file);
int addFile(char* path, int check);
int removeFile(char* path);
void decreaseReaderCount(File* file);
void openWriteLock(File* file);
void cleanUpFileStruct();

// ACTUAL FILE FUNCTIONS
int checkFileType(char* path);
int fileExists(char *filename);
void createFile(char* path, char* response);
void deleteFile(char *filename, char* response);
void getFileMetaData(char* filepath, int clientSocketID);
void copyFile(char* ss_path, char* response);
void copyDirectory(char* ss_path, char* response);

#endif