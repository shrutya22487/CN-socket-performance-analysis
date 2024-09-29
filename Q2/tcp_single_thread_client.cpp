#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>


#define SERVER_IP "127.0.0.1" // change to "10.0.0.1" if on VM, "127.0.0.1" if on local machine
#define PORT 8080
#define BUFFER_SIZE 1024

// TODO: #1 Error handling
int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char message[] = "Requesting top 2 processes";
    char buffer[BUFFER_SIZE] = {0};

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        std::cerr << "Socket creation error" << std::endl;
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr) <= 0) {
        std::cerr << "Invalid address/Address not supported" << std::endl;
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        std::cerr << "Connection Failed" << std::endl;
        return -1;
    }

    send(sock, message, strlen(message), 0);
    printf("Client -> Top 2 processes Request Sent\n");

    read(sock, buffer, sizeof(buffer));
    printf("Client -> Message recieved:\n%s\n", buffer);

    close(sock);
    return 0;
}
