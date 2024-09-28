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

void client_task(int thread_id) {
    int sock;
    struct sockaddr_in server_addr;
    char message[BUFFER_SIZE];
    char server_reply[BUFFER_SIZE];

    // Create socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("Could not create socket");
        exit(1);
    }
    printf("Thread %d: Socket created.\n", thread_id);

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, IP_ADDRESS_OF_SERVER.c_str(), &server_addr.sin_addr) <= 0) {
        perror("Invalid address or address not supported");
        close(sock);
        exit(1);
    }

    // Connect to the server
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        close(sock);
        exit(1);
    }
    printf("Thread %d: Connected to server.\n", thread_id);

    // Prepare the message
    sprintf(message, "Hello from client %d", thread_id);

    // Send the message to the server
    if (send(sock, message, strlen(message), 0) < 0) {
        perror("Send failed");
        close(sock);
        exit(1);
    }
    printf("Thread %d: Message sent: %s\n", thread_id, message);

    // Receive a reply from the server
    if (recv(sock, server_reply, BUFFER_SIZE, 0) < 0) {
        perror("Receive failed");
    } else {
        server_reply[strlen(server_reply)] = '\0'; // Null-terminate the received message
        printf("Thread %d: Server reply: %s\n", thread_id, server_reply);
    }

    // Close the socket
    close(sock);
    printf("Thread %d: Connection closed.\n", thread_id);
}

int main(int argc, char *argv[]) {
    // Check if the number of threads is provided as an argument
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <num_threads>\n", argv[0]);
        exit(1);
    }

    // Convert the argument to an integer
    int num_threads = atoi(argv[1]);
    if (num_threads <= 0) {
        fprintf(stderr, "Please provide a positive integer for the number of threads.\n");
        exit(1);
    }

    std::vector<std::thread> threads;

    // Create the specified number of client threads
    for (int i = 0; i < num_threads; ++i) {
        threads.push_back(std::thread(client_task, i));
    }

    // Join all threads to ensure the main program waits for them to finish
    for (auto &t : threads) {
        t.join();
    }

    return 0;
}