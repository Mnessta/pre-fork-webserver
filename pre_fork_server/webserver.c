#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/wait.h>

#define PORT 8080
#define BACKLOG 10
#define MAX_CHILDREN 100
#define MAX_REQUESTS_PER_CHILD 5

int active_children = 0;
int child_pids[MAX_CHILDREN];

/**
 * Handle SIGCHLD signal to reap zombie processes
 */
void sigchld_handler(int s) {
    while (waitpid(-1, NULL, WNOHANG) > 0)
        active_children--;
}

/**
 * Handles a single client request
 */
void handle_client(int client_sock) {
    char buffer[1024];
    memset(buffer, 0, sizeof(buffer));

    read(client_sock, buffer, sizeof(buffer) - 1);
    printf("Child %d received: %s\n", getpid(), buffer);

    sleep(1); // Simulate processing delay

    // Send simple HTTP response
    char *response = "HTTP/1.1 200 OK\r\nContent-Length: 13\r\n\r\nHello, World!";
    write(client_sock, response, strlen(response));

    close(client_sock);
}

/**
 * Each child process runs this loop to accept and handle connections
 */
void child_process(int server_sock, int max_requests) {
    int handled = 0;

    while (handled < max_requests) {
        struct sockaddr_in client;
        socklen_t len = sizeof(client);
        int client_sock = accept(server_sock, (struct sockaddr *)&client, &len);
        if (client_sock < 0) {
            perror("accept");
            continue;
        }

        printf("Child %d handling client %s:%d\n", getpid(),
               inet_ntoa(client.sin_addr), ntohs(client.sin_port));

        handle_client(client_sock);
        handled++;
    }

    printf("Child %d handled %d requests. Exiting.\n", getpid(), handled);
    exit(0);
}

/**
 * Main server: creates child pool and accepts traffic
 */
int main(int argc, char *argv[]) {
    int server_sock;
    struct sockaddr_in server;

    // Set up signal handler for SIGCHLD
    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGCHLD, &sa, NULL);

    // Create socket
    if ((server_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }

    // Allow reuse of address
    int yes = 1;
    setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

    // Bind socket to port
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    server.sin_addr.s_addr = INADDR_ANY;
    memset(&(server.sin_zero), '\0', 8);

    if (bind(server_sock, (struct sockaddr *)&server, sizeof(server)) == -1) {
        perror("bind");
        exit(1);
    }

    if (listen(server_sock, BACKLOG) == -1) {
        perror("listen");
        exit(1);
    }

    printf("Parent process %d: Listening on port %d\n", getpid(), PORT);

    // Fork initial children
    int num_children = 5;
    for (int i = 0; i < num_children; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            // In child process
            child_process(server_sock, MAX_REQUESTS_PER_CHILD);
        } else if (pid > 0) {
            // In parent
            child_pids[active_children++] = pid;
        } else {
            perror("fork");
        }
    }

    // Parent waits for Ctrl+C
    while (1) {
        pause();
    }

    return 0;
}

