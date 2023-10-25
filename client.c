#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8000
#define BUFFER_SIZE 1024

int main() {
    int client_socket;
    struct sockaddr_in server_address;
    char buffer[BUFFER_SIZE];

    // Creating socket file descriptor
    if ((client_socket = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, "127.0.0.1", &server_address.sin_addr) <= 0) {
        perror("Invalid address/ Address not supported");
        exit(EXIT_FAILURE);
    }

    if (connect(client_socket, (struct sockaddr*)&server_address, sizeof(server_address)) < 0) {
        perror("Connection Failed");
        exit(EXIT_FAILURE);
    }

    FILE* file = fopen("sample_file.txt", "a");
    if (file == NULL) {
        perror("File open failed");
        exit(EXIT_FAILURE);
    }

    /* Receiving data from the server*/
    int bytes_received;
    while((bytes_received = recv(client_socket, buffer, sizeof(buffer), 0)) > 0) {
        printf("%s\n", buffer);
        if(fwrite(buffer,bytes_received, sizeof(buffer), file) < 0){
            perror("Error writing to file");
            fclose(file);
            break;
        }
    }

    /* Sending data to server
    int bytes_read;
    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, file)) > 0) {
        send(client_socket, buffer, bytes_read, 0);
    }
    */

    fclose(file);
    close(client_socket);

    printf("File sent successfully.\n");

    return 0;
}