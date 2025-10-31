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

    // Read initial greeting/menu before the main loop
    while (1) {
        memset(buffer, 0, sizeof(buffer));
        ssize_t bytes = read(sock, buffer, sizeof(buffer));
        if (bytes <= 0) {
            printf("Server closed the connection.\n");
            close(sock);
            return 0;
        }
        
        // Use standard output to print the initial greeting
        printf("%s", buffer);
        fflush(stdout); // Initial flush is fine here
        
        if (strstr(buffer, "Enter your choice")) {
            break;
        }
    }

    while (1) {
        /* Read menu choice from stdin using fgets and strtol so non-numeric
           input won't cause the client to exit unexpectedly. */
        char line[64];
        while (1) {
            printf("> ");
            fflush(stdout);
            if (!fgets(line, sizeof(line), stdin)) {
                printf("Input error. Exiting.\n");
                goto cleanup;
            }

            char *endptr;
            long val = strtol(line, &endptr, 10);
            while (*endptr == ' ' || *endptr == '\t') endptr++;
            if (endptr == line || (*endptr != '\n' && *endptr != '\0')) {
                printf("Invalid input. Please enter a number.\n");
                continue;
            }
            choice = (int)val;
            break;
        }

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

                /* Write server output unbuffered so prompts are displayed
                    even if stdout is line-buffered. */
                write(STDOUT_FILENO, buffer, bytes);

            // If the server is prompting for input (colon or >)
            if (strstr(buffer, ":") || strstr(buffer, ">")) {
                char input[256];
                printf("> ");
                fflush(stdout); // Flush after client prompt
                if (!fgets(input, sizeof(input), stdin)) {
                    printf("Input error. Exiting.\n");
                    goto cleanup;
                }
                input[strcspn(input, "\n")] = '\0';
                write(sock, input, strlen(input) + 1);
                continue;  // keep looping to read next message
            }

            // If we reached the main menu again (detected by "Enter your choice")
            if (strstr(buffer, "Enter your choice")) {
                break; // safely exit to outer loop, which will re-display the menu
            }
        }
    }

cleanup:
    close(sock);
    return 0;
}