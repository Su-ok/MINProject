#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<arpa/inet.h>
#include<pthread.h>
#include"handler.h"

#define PORT 8080
#define MAX_CLIENTS 20

int main() {
    int server_fd, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);
    pthread_t tid;

    // Create socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Allow immediate reuse of address/port
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Bind socket to port
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_fd, MAX_CLIENTS) < 0) {
        perror("Listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("\n===========================================\n");
    printf("     Welcome to SWIFT Bank Server System  \n");
    printf("   Server is up and listening on port %d...\n", PORT);
    printf("===========================================\n\n");


    // Accept and handle clients
    while (1) {
        client_sock = accept(server_fd, (struct sockaddr *)&client_addr, &addr_len);
        if (client_sock < 0) {
            perror("Accept failed");
            continue;
        }

        printf("New client connected.\n");
        const char *greet = "\nWelcome to SWIFT Bank!\nYour trusted digital banking partner.\n-------------------------------------\n";
        write(client_sock, greet, strlen(greet));

        int *new_sock = malloc(sizeof(int));
        *new_sock = client_sock;

        if (pthread_create(&tid, NULL, handle_client, (void *)new_sock) != 0) {
            perror("pthread_create");
            close(client_sock);
            free(new_sock);
            continue;
        }

        pthread_detach(tid); // auto-cleanup threads
    }

    close(server_fd);
    return 0;
}
