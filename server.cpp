#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <stdlib.h>   // Include for malloc
#define PORT 8080
#define BUFFER_SIZE 1024

void *handle_client(void *socket_desc) {
    int client_sock = *(int *)socket_desc;
    char buffer[BUFFER_SIZE];
    int read_size;

    // Receive a message from the client
    while ((read_size = recv(client_sock, buffer, BUFFER_SIZE, 0)) > 0) {
        // End of string marker
        buffer[read_size] = '\0';
        printf("Received from client: %s\n", buffer);

        // Send the message back to the client (echo)
        send(client_sock, buffer, strlen(buffer), 0);
    }

    if (read_size == 0) {
        printf("Client disconnected.\n");
    } else if (read_size == -1) {
        perror("recv failed");
    }

    // Free the socket pointer and close the connection
    close(client_sock);
    free(socket_desc);

    return NULL;
}

int main() {
    int server_sock, client_sock, *new_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_size = sizeof(struct sockaddr_in);

    // Create socket
    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock == -1) {
        perror("Could not create socket");
        return 1;
    }
    printf("Socket created.\n");

    // Prepare the sockaddr_in structure
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;  // Accept connections on any IP address
    server_addr.sin_port = htons(PORT);

    // Bind the socket
    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_sock);
        return 1;
    }
    printf("Bind done.\n");

    // Listen for incoming connections
    listen(server_sock, 5);
    printf("Waiting for incoming connections...\n");

    while ((client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &addr_size))) {
        printf("Connection accepted.\n");

        pthread_t client_thread;
        new_sock = (int *)malloc(sizeof(int));  // Allocate memory for the socket
        if (new_sock == NULL) {
            perror("Could not allocate memory");
            return 1;
        }
        *new_sock = client_sock;
        printf("Handler assigned.\n");
        handle_client(new_sock);
    }

    if (client_sock < 0) {
        perror("Accept failed");
        return 1;
    }

    // Close the socket
    close(server_sock);

    return 0;
}
