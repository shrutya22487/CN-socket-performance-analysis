#include <iostream>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/select.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <ctype.h> 
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT 8080
#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

struct process_info {
    int pid;
    char name[256];
    unsigned long long user_cpu_time;
    unsigned long long kernel_cpu_time;
    unsigned long long cpu_time;  // user + kernel time in clock ticks
};

unsigned long long get_process_cpu_time(int pid, struct process_info *proc) {
    char stat_path[128];
    sprintf(stat_path, "/proc/%d/stat", pid);

    FILE *stat_file = fopen(stat_path, "r");
    if (!stat_file) {
        return 0;
    }

    char buffer[512];
    if (fgets(buffer, sizeof(buffer), stat_file) == NULL) {
        fclose(stat_file);
        return 0;
    }

    char *fields[52]; 
    char *tok = strtok(buffer, " ");
    int idx = 0;

    while (tok != NULL && idx < 52) {
        fields[idx++] = tok;
        tok = strtok(NULL, " ");
    }

    proc->pid = atoi(fields[0]);

    snprintf(proc->name, sizeof(proc->name), "%s", fields[1]);
    size_t name_len = strlen(proc->name);
    if (proc->name[0] == '(' && proc->name[name_len - 1] == ')') {
        proc->name[name_len - 1] = '\0';
        memmove(proc->name, proc->name + 1, name_len - 1);
    }

    proc->user_cpu_time = strtoul(fields[13], NULL, 10);
    proc->kernel_cpu_time = strtoul(fields[14], NULL, 10);

    proc->cpu_time = proc->user_cpu_time + proc->kernel_cpu_time;

    fclose(stat_file);
    return proc->cpu_time;
}

void get_top_two_processes(struct process_info *top_processes) {
    struct dirent *entry;
    DIR *proc_dir = opendir("/proc");

    if (!proc_dir) {
        perror("opendir /proc error.\n");
        return;
    }

    struct process_info p1;
    struct process_info p2;
    
    unsigned long long maxi1 = 0;
    unsigned long long maxi2 = 0;

    while ((entry = readdir(proc_dir)) != NULL) {
        if (entry->d_type == DT_DIR) {
            const char *pid_str = entry->d_name;
            int is_pid = 1;
            for (int i = 0; pid_str[i] != '\0'; i++) {
                if (!isdigit(pid_str[i])) {
                    is_pid = 0;
                    break;
                }
            }
            if (is_pid) {
                int pid = atoi(pid_str);
                struct process_info proc;
                proc.pid = pid;
                proc.cpu_time = get_process_cpu_time(pid, &proc);

                if (proc.cpu_time > 0) {
                    if (proc.cpu_time > maxi1) {
                        p2 = p1;
                        p1 = proc;
                        maxi2 = maxi1;
                        maxi1 = proc.cpu_time;
                    } else if (proc.cpu_time > maxi2) {
                        p2 = proc;
                        maxi2 = proc.cpu_time;
                    }
                }
            }
        }
    }

    closedir(proc_dir);

    top_processes[0] = p1;
    top_processes[1] = p2;
}

int main() {
    int server_fd, new_socket, client_sockets[MAX_CLIENTS], max_sd, sd;
    struct sockaddr_in address;
    fd_set readfds;
    char buffer[BUFFER_SIZE];
    int addrlen = sizeof(address);

    // Initialize client sockets
    for (int i = 0; i < MAX_CLIENTS; i++) {
        client_sockets[i] = 0;
    }

    // Create server socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }
    else printf("Server Socket created successfully.\n");

    // Set socket options
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("Setsockopt failed");
        exit(EXIT_FAILURE);
    }

    // Bind the socket to the address and port
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }
    else printf("Server Socket binded successfully.\n");

    // Listen for incoming connections
    if (listen(server_fd, 3) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }
    else printf("Server listening for incoming connection requests.\n");

    while (true) {
        // Clear the socket set
        FD_ZERO(&readfds);

        // Add server socket to set
        FD_SET(server_fd, &readfds);
        max_sd = server_fd;

        // Add client sockets to set
        for (int i = 0; i < MAX_CLIENTS; i++) {
            sd = client_sockets[i];
            if (sd > 0) {
                FD_SET(sd, &readfds);
            }
            if (sd > max_sd) {
                max_sd = sd;
            }
        }

        // Wait for activity on any of the sockets
        int activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);
        if ((activity < 0) && (errno != EINTR)) {
            perror("Select error");
        }

        // If something happened on the server socket, it's an incoming connection
        if (FD_ISSET(server_fd, &readfds)) {
            if ((new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen)) < 0) {
                perror("Accept failed");
                exit(EXIT_FAILURE);
            }

            std::cout << "New connection, socket fd is " << new_socket << std::endl;

            // Add new socket to the client_sockets array
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (client_sockets[i] == 0) {
                    client_sockets[i] = new_socket;
                    std::cout << "Adding to list of sockets as " << i << std::endl;
                    break;
                }
            }
        }

        // Check all client sockets
        for (int i = 0; i < MAX_CLIENTS; i++) {
            sd = client_sockets[i];
            if (FD_ISSET(sd, &readfds)) {
        
                int val_read = read(sd, buffer, BUFFER_SIZE);
                if (val_read < 0) {
                    perror("Read error.\n");
                    close(sd);
                    return 1;
                }
                else if(val_read > 0){
                    buffer[val_read] = '\0';
                    printf("Received request from client: %s\n", buffer);
                }

                if (val_read == 0) {
                    // Client disconnected
                    getpeername(sd, (struct sockaddr*)&address, (socklen_t*)&addrlen);
                    std::cout << "Host disconnected, IP " << inet_ntoa(address.sin_addr)
                              << ", port " << ntohs(address.sin_port) << std::endl;

                    close(sd);
                    client_sockets[i] = 0;
                } 
                else if (strcmp(buffer, "Requesting top 2 processes") == 0) {
                    
                    memset(buffer, 0, BUFFER_SIZE);                   
                    struct process_info top_processes[2];
                    get_top_two_processes(top_processes);

                    sprintf(buffer, "Process 1: PID=%d, Name=%s, CPU Time=%llu\n"
                                    "Process 2: PID=%d, Name=%s, CPU Time=%llu\n",
                            top_processes[0].pid, top_processes[0].name, top_processes[0].cpu_time,
                            top_processes[1].pid, top_processes[1].name, top_processes[1].cpu_time);
                    send(sd, buffer, strlen(buffer), 0);
                }
                else {
                    memset(buffer, 0, BUFFER_SIZE);
                    sprintf(buffer, "Unsupported request");
                    send(sd, buffer, strlen(buffer), 0);
                }

            }
        }
    }

    return 0;
}
