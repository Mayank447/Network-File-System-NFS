#include "header_files.h"

char Directory_Msg[BUFFER_LENGTH];


// Function to create a directory
void createDirectory(char* path, char* response)
{
    char path_copy[300];
    strcpy(path_copy,path); // Create a copy of the path

    char *token = strtok(path_copy, "/"); // Tokenize the path
    char current_dir[256]; // Initialize a buffer for the current directory
    int global_count = 0;

    pid_t childPid = fork();
    if (childPid < 0) {
        perror("[-] Error creating child process");
        sprintf(response, "%d", STORAGE_SERVER_ERROR);
        return;
    }

    else if (childPid == 0)
    {
        // This is the child process
        while (token != NULL)
        {
            // Append the next directory to the current directory
            strcat(current_dir, token);
            global_count++;

            // Check if the directory already exists
            struct stat st;
            if (stat(token, &st) == -1) {
                if (mkdir(token, 0777) != 0) {
                    sprintf(response, "%d", ERROR_DIRECTORY_ALREADY_EXISTS);
                    exit(EXIT_FAILURE);
                }
            }

            if (chdir(token) != 0) {
                sprintf(response, "%d", STORAGE_SERVER_ERROR);
                exit(EXIT_FAILURE);
            }
            // Add a slash to separate directories in the current directory string
            strcat(current_dir, "/");
            token = strtok(NULL, "/");
        }

        while (global_count > 0) {
            if (chdir("..") != 0) {
                sprintf(response, "%d", STORAGE_SERVER_ERROR);
                exit(EXIT_FAILURE);
            }
            global_count--;
        }
        exit(EXIT_SUCCESS);
    }

    else {
        // This is the parent process
        int status;
        waitpid(childPid, &status, 0);

        if (WIFEXITED(status)) 
            strcpy(response, VALID_STRING);
        else{
            sprintf(response, "%d", STORAGE_SERVER_ERROR);
        }
    }
}


void deleteDirectory(char* path, char* response)
{
    char path_copy[300];
    strcpy(path_copy,path); // Create a copy of the path

    char *token = strtok(path_copy, "/"); // Tokenize the path
    char current_dir[256]; // Initialize a buffer for the current directory

    pid_t childPid = fork();
    if (childPid < 0) {
        perror("[-] Error creating child process");
        sprintf(response, "%d", STORAGE_SERVER_ERROR);
        return;
    }

    else if (childPid == 0)
    {
        // This is the child process
        while (token != NULL)
        {
            // Append the next directory to the current directory
            strcat(current_dir, token);

            // Check if the directory already exists
            struct stat st;
            if (stat(token, &st) == -1) {
                sprintf(response, "%d", ERROR_DIRECTORY_DOES_NOT_EXIST);
                exit(EXIT_FAILURE);
            }

            if(token != NULL) {
                strcpy(current_dir, token);
                if(chdir(token) != 0) {
                    sprintf(response, "%d", STORAGE_SERVER_ERROR);
                    exit(EXIT_FAILURE);
                }
            }
            token = strtok(NULL, "/");
        }

        if(token == NULL){
           chdir("..");
           rmdir(current_dir);
            exit(EXIT_SUCCESS);
        }
        exit(EXIT_FAILURE);
    }

    else {
        // This is the parent process
        int status;
        waitpid(childPid, &status, 0);

        if (WIFEXITED(status)) 
            strcpy(response, VALID_STRING);
        else{
            sprintf(response, "%d", STORAGE_SERVER_ERROR);
        }
    }
}

/*
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
                    // copyFile(sourcePath, destinationPath);
                }
            }
        }
    }
    closedir(dir);
}
*/