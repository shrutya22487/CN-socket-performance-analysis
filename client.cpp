#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 1024

int main() {
    int sock;
    struct sockaddr_in server_addr;
    char message[BUFFER_SIZE], server_reply[BUFFER_SIZE];

    // Create socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("Could not create socket");
        return 1;
    }
    printf("Socket created.\n");

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, "192.168.1.2", &server_addr.sin_addr) <= 0) {
        perror("Invalid address or address not supported");
        return -1;
    }

    // Connect to the server
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        return 1;
    }
    printf("Connected to server.\n");

    // Communicate with the server
    while (1) {
        printf("Enter message: ");
        fgets(message, BUFFER_SIZE, stdin);

        // Send the message to the server
        if (send(sock, message, strlen(message), 0) < 0) {
            perror("Send failed");
            return 1;
        }

        // Receive a reply from the server
        if (recv(sock, server_reply, BUFFER_SIZE, 0) < 0) {
            perror("Recv failed");
            break;
        }

        printf("Server reply: %s\n", server_reply);
    }

    // Close the socket
    close(sock);

    return 0;
}
