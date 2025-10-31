// client.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 2048

int main() {
    int sock;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    int choice;

    // Create socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // Connect to the server
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        close(sock);
        exit(EXIT_FAILURE);
    }

    printf("Connected to Banking Management Server.\n");

    while (1) {
        memset(buffer, 0, sizeof(buffer));
        // Use read() to get the greeting and first menu
        ssize_t bytes = read(sock, buffer, sizeof(buffer));
        if (bytes <= 0) {
            printf("Server closed the connection.\n");
            close(sock);
            return 0;
        }
        printf("%s", buffer);
        // Wait for the server to show the menu text before prompting for choice
        if (strstr(buffer, "Enter your choice")) {
            break;
        }
    }

    while (1) {
        // Get user's menu choice
        printf("> ");
        if (scanf("%d", &choice) != 1) {
            printf("Invalid input. Exiting.\n");
            break;
        }

        // Clear input buffer (remove stray newline or leftover bytes)
        int ch;
        while ((ch = getchar()) != '\n' && ch != EOF);

        char choice_str[16];
        snprintf(choice_str, sizeof(choice_str), "%d", choice);

        // Send the choice as a STRING, including the null terminator
        write(sock, choice_str, strlen(choice_str) + 1);


        if (choice == 5) {
            printf("Logging out...\n");
            break;
        }

        // Keep reading server messages
        while (1) {
            memset(buffer, 0, sizeof(buffer));
            ssize_t bytes = read(sock, buffer, sizeof(buffer));
            if (bytes <= 0)
                break;

            printf("%s", buffer);

            // If the server is prompting for input (colon or >)
            if (strstr(buffer, ":") || strstr(buffer, ">")) {
                char input[256];
                printf("> ");
                scanf("%s", input);
                write(sock, input, strlen(input) + 1);
                continue;  // keep looping to read next message
            }

            // If we reached the main menu again (detected by "Enter your choice")
            if (strstr(buffer, "Enter your choice")) {
                break; // safely exit to outer loop, which will re-display the menu
            }
        }
    }

    close(sock);
    return 0;
}
