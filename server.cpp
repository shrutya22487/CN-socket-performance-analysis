#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#define PORT 8081
#define BUFFER_SIZE 1024
#define MAX_PROCESSES 100

struct process_info {
    char name[256];
    int pid;
    long unsigned int user_time;
    long unsigned int kernel_time;
};

// Function to read process information from /proc/[pid]/stat
void read_process_info(struct process_info *proc, int pid) {
    char stat_path[BUFFER_SIZE];
    sprintf(stat_path, "/proc/%d/stat", pid);
    
    FILE *stat_file = fopen(stat_path, "r");
    if (stat_file == NULL) {
        perror("fopen failed");
        return;
    }

    // The format of /proc/[pid]/stat is quite complex. We're interested in the following fields:
    // 1) pid (field 1)
    // 2) process name (field 2)
    // 3) user time (field 14)
    // 4) kernel time (field 15)
    int temp_pid;
    char temp_name[256];
    long unsigned int temp_utime, temp_ktime;

    fscanf(stat_file, "%d %s %*c %*d %*d %*d %*d %*d %*u %*u %*u %*u %*u %lu %lu",
           &temp_pid, temp_name, &temp_utime, &temp_ktime);

    // Remove parentheses from the process name
    strcpy(proc->name, temp_name + 1);
    proc->name[strlen(proc->name) - 1] = '\0';
    proc->pid = temp_pid;
    proc->user_time = temp_utime;
    proc->kernel_time = temp_ktime;

    fclose(stat_file);
}

// Function to get the top two CPU-consuming processes
void get_top_two_cpu_processes(struct process_info *top_two) {
    DIR *proc_dir = opendir("/proc");
    struct dirent *entry;
    struct process_info processes[MAX_PROCESSES];
    int proc_count = 0;

    if (proc_dir == NULL) {
        perror("opendir failed");
        return;
    }

    // Iterate over all directories in /proc
    while ((entry = readdir(proc_dir)) != NULL) {
        // If the directory name is a number (PID)
        if (isdigit(*entry->d_name)) {
            int pid = atoi(entry->d_name);
            struct process_info proc;
            read_process_info(&proc, pid);
            processes[proc_count++] = proc;
        }
    }

    closedir(proc_dir);

    // Sort processes by (user_time + kernel_time) in descending order
    for (int i = 0; i < proc_count - 1; i++) {
        for (int j = 0; j < proc_count - i - 1; j++) {
            long unsigned int cpu_time_j = processes[j].user_time + processes[j].kernel_time;
            long unsigned int cpu_time_j1 = processes[j + 1].user_time + processes[j + 1].kernel_time;

            if (cpu_time_j < cpu_time_j1) {
                struct process_info temp = processes[j];
                processes[j] = processes[j + 1];
                processes[j + 1] = temp;
            }
        }
    }

    // Store the top two CPU-consuming processes
    top_two[0] = processes[0];
    top_two[1] = processes[1];
}

void *handle_client(void *socket_desc) {
    int client_sock = *(int *)socket_desc;
    char buffer[BUFFER_SIZE];
    int read_size;

    // Receive a message from the client
    while ((read_size = recv(client_sock, buffer, BUFFER_SIZE, 0)) > 0) {
        buffer[read_size] = '\0';
        printf("Received from client: %s\n", buffer);

        if (strcmp(buffer, "GET_TOP_CPU") == 0) {
            struct process_info top_two[2];
            get_top_two_cpu_processes(top_two);

            char response[BUFFER_SIZE];
            sprintf(response, "Top CPU Processes:\n1. Name: %s, PID: %d, User Time: %lu, Kernel Time: %lu\n"
                              "2. Name: %s, PID: %d, User Time: %lu, Kernel Time: %lu\n",
                    top_two[0].name, top_two[0].pid, top_two[0].user_time, top_two[0].kernel_time,
                    top_two[1].name, top_two[1].pid, top_two[1].user_time, top_two[1].kernel_time);

            // Send the message back to the client
            send(client_sock, response, strlen(response), 0);
        } else {
            send(client_sock, "Invalid request", 15, 0);
        }
    }

    if (read_size == 0) {
        printf("Client disconnected.\n");
    } else if (read_size == -1) {
        perror("recv failed");
    }

    close(client_sock);
    free(socket_desc);
    return NULL;
}

int main() {
    int server_sock, client_sock, *new_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_size = sizeof(struct sockaddr_in);

    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock == -1) {
        perror("Could not create socket");
        return 1;
    }
    printf("Socket created.\n");

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_sock);
        return 1;
    }
    printf("Bind done.\n");

    if (listen(server_sock, 5)) {
        std::cerr << "Could not listen on socket" << std::endl;
        exit(1);
    }

    printf("Waiting for incoming connections...\n");

    while ((client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &addr_size))) {
        printf("Connection accepted.\n");

        pthread_t client_thread;
        new_sock = (int *)malloc(sizeof(int));
        if (new_sock == NULL) {
            perror("Could not allocate memory");
            return 1;
        }
        *new_sock = client_sock;

        if (pthread_create(&client_thread, NULL, handle_client, (void *)new_sock) < 0) {
            perror("Could not create thread");
            return 1;
        }
        printf("Handler assigned.\n");

        pthread_detach(client_thread);
    }

    if (client_sock < 0) {
        perror("Accept failed");
        return 1;
    }

    close(server_sock);
    return 0;
}
