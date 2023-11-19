#include "header_files.h"

char Msg[BUFFER_LENGTH];

/*
// Function to create a directory
void createDirectory(char* path, int clientSocket);


// Function to copy directories between two storage servers
void copyDirectory(const char *sourceDir, const char *destinationDir) {
    struct dirent *entry;
    struct stat statInfo;
    char sourcePath[MAX_PATH_LENGTH], destinationPath[MAX_PATH_LENGTH];
    // Open source directory
    DIR *dir = opendir(sourceDir);
    if (dir == NULL) {
        perror("[-] Unable to open source directory");
        return;
    }
    // Create destination directory
    mkdir(destinationDir, 0777);
    while ((entry = readdir(dir))) {
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            sprintf(sourcePath, "%s/%s", sourceDir, entry->d_name);
            sprintf(destinationPath, "%s/%s", destinationDir, entry->d_name);
            if (stat(sourcePath, &statInfo) == 0) {
                if (S_ISDIR(statInfo.st_mode)) {
                    // Recursively copy subdirectories
                    copyDirectory(sourcePath, destinationPath);
                } else {
                    // Copy files
                    copyFile(sourcePath, destinationPath);
                }
            }
        }
    }
    closedir(dir);
}


void deleteDirectory(const char *path, int clientSocketID)
{
    DIR *dir;
    struct stat stat_path;
    struct dirent *entry;
    // stat for the path
    stat(path, &stat_path);

    // if path does not exists or is not dir - exit with status -1
    if (S_ISDIR(stat_path.st_mode) == 0)
    {
        bzero(Msg, ERROR_BUFFER_LENGTH);
        sprintf(Msg, "Is not directory: %s\n", path);
        if (send(clientSocketID, Msg, sizeof(Msg), 0) < 0)
        {
            perror("[-] Unable to send: Is not directory");
        }
        return;
    }

    // if not possible to read the directory for this user
    if ((dir = opendir(path)) == NULL)
    {
        bzero(Msg, ERROR_BUFFER_LENGTH);
        sprintf(Msg, "Can`t open directory: %s\n", path);
        if (send(clientSocketID, Msg, strlen(Msg), 0) < 0)
        {
            perror("[-] Unable to send: Can`t open directory");
        }
        return;
    }

    while ((entry = readdir(dir)) != NULL)
    {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
        {
            continue; // Skip current and parent directories
        }

        char entryPath[MAX_PATH_LENGTH];
        snprintf(entryPath, sizeof(entryPath), "%s/%s", path, entry->d_name);

        if (entry->d_type == 4)
        { // Recursive call for subdirectories
            deleteDirectory(entryPath, clientSocketID);
        }

        else
        {
            deleteFile(entryPath, clientSocketID); // Delete files

            if (remove(entryPath) != 0)
            {
                bzero(Msg, ERROR_BUFFER_LENGTH);
                sprintf(Msg, "Error deleting file: %s", entryPath);
                if (send(clientSocketID, Msg, strlen(Msg), 0) < 0)
                {
                    printf("%s ", Msg);
                    perror("[-] Error sending message:");
                }
            }
        }
    }
    closedir(dir);

    // Remove the empty directory after deleting its contents
    if (rmdir(path) != 0)
    {
        bzero(Msg, ERROR_BUFFER_LENGTH);
        sprintf(Msg, "Error: Deleting the repository %s", path);
        if (send(clientSocketID, Msg, strlen(Msg), 0) < 0)
        {
            printf("%s ", Msg);
            perror("[-] Error sending message:");
        }
    }
}

*/
