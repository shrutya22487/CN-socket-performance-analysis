#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <thread>
#include <unistd.h>
#include <vector>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 1024
std::string IP_ADDRESS_OF_SERVER = "192.168.1.2";
#define NUM_THREADS 5
void client_task() {
    int sock;
    struct sockaddr_in server_addr;
    char message[BUFFER_SIZE];

    // Create socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("Could not create socket");
        exit(1);
    }
    printf("Socket created.\n");

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, IP_ADDRESS_OF_SERVER.c_str(), &server_addr.sin_addr) <= 0) {
        perror("Invalid address or address not supported");
        exit(1);
    }

    // Connect to the server
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        exit(1);
    }
    printf("Connected to server.\n");

    // Communicate with the server
    while (1) {
        printf("Enter message: ");
        fgets(message, BUFFER_SIZE, stdin);

        // Send the message to the server
        if (send(sock, message, strlen(message), 0) < 0) {
            perror("Send failed");
            exit(1);
        }
        printf("Message sent.\n, ending connection \n");
        break;
        // Receive a reply from the server
        // if (recv(sock, server_reply, BUFFER_SIZE, 0) < 0) {
        //     perror("Recv failed");
        //     break;
        // }
        //
        // printf("Server reply: %s\n", server_reply);
    }

    // Close the socket
    close(sock);
}

int main() {
    std::vector<std::thread> threads;

    // Create the specified number of client threads
    for (int i = 0; i < NUM_THREADS; ++i) {
        threads.push_back(std::thread(client_task));
    }

    // Join all threads to ensure the main program waits for them to finish
    for (auto &t : threads) {
        t.join();
    }

    return 0;
}