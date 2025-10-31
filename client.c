// client.c
/* #define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>

#define PORT 8080
#define BUFFER_SIZE 4096
#define PROMPT_TOKEN "Enter your choice"

static ssize_t safe_read(int fd, void *buf, size_t count) {
    ssize_t r = read(fd, buf, count);
    if (r < 0 && errno == EINTR) return 0;
    return r;
}

// Read from socket until we see a token indicating the server is asking for choice.
// Print everything we get. Return 0 on success, -1 on socket closed/error.
int drain_until_prompt(int sock) {
    char buf[BUFFER_SIZE];
    ssize_t n;
    int got_prompt = 0;

    // We'll loop and print whatever the server sends until we detect the prompt string.
    // To avoid blocking forever if server only sends greeting then menu in multiple writes,
    // we keep reading until the prompt token appears.
    while (!got_prompt) {
        memset(buf, 0, sizeof(buf));
        n = safe_read(sock, buf, sizeof(buf)-1);
        if (n == 0) {
            // server closed connection
            fprintf(stderr, "Server closed connection (drain).\n");
            return -1;
        }
        if (n < 0) {
            perror("read");
            return -1;
        }
        // Print whatever arrived
        fwrite(buf, 1, n, stdout);
        fflush(stdout);

        // If server included the prompt token, stop
        if (strstr(buf, PROMPT_TOKEN) != NULL || strstr(buf, "Enter") != NULL) {
            // If we match any Enter/Username/Password prompt, stop and let user respond.
            got_prompt = 1;
            break;
        }

        // If server sent smaller chunk we continue loop; break only when token found.
        // To avoid busy loop, if less than buffer and no prompt, try another read (server may send more).
    }
    return 0;
}

// Read one line from stdin, strip newline. Returns buf pointer or NULL on EOF.
char *read_user_line(char *buf, size_t sz) {
    if (!fgets(buf, (int)sz, stdin)) return NULL;
    size_t L = strlen(buf);
    if (L && (buf[L-1] == '\n' || buf[L-1] == '\r')) buf[--L] = '\0';
    // remove trailing \r if any
    if (L && buf[L-1] == '\r') buf[--L] = '\0';
    return buf;
}

int main(void) {
    int sock;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) { perror("socket"); return 1; }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        close(sock);
        return 1;
    }

    printf("Connected to Banking Management Server.\n\n");

    // Read initial greeting + menu (server may send both). Drain until prompt appears.
    if (drain_until_prompt(sock) < 0) { close(sock); return 1; }

    while (1) {
        // Prompt token indicates server expects a numeric menu choice.
        // Read a line from user and parse integer.
        printf("> ");
        fflush(stdout);

        char line[256];
        if (!read_user_line(line, sizeof(line))) { // EOF on stdin
            printf("\nInput closed, exiting.\n");
            break;
        }

        // Try parse int choice
        char *endptr;
        long choice = strtol(line, &endptr, 10);
        if (endptr == line || *endptr != '\0') {
            // not a pure integer — user might have typed something else, send as string?
            // but server expects int for menu choices: ask again
            printf("Please enter a number for the menu choice.\n");
            continue;
        }

        int ch = (int)choice;
        // Send binary int as server expects
        ssize_t w = write(sock, &ch, sizeof(ch));
        if (w != sizeof(ch)) {
            perror("write(choice)");
            break;
        }

        // If user chose Exit (5) then either server will close or respond; we'll attempt to read and then exit.
        if (ch == 5) {
            // read final server messages (goodbye)
            if (drain_until_prompt(sock) < 0) break;
            break;
        }

        // After sending the menu choice, server will send a prompt flow: e.g. "Username:", or further menu.
        // Drain and print until a server prompt or "Enter your choice" appears again.
        if (drain_until_prompt(sock) < 0) break;

        // Now server may be asking for username/password or other inputs.
        // We should handle different server prompts generically:
        // If server asks "Username:" or "Password:" or any ":" prompt, we read a line and send it as a NUL-terminated string.

        // We'll loop reading server messages and react to any colon prompt; break back to menu when "Enter your choice" returns.

        while (1) {
            // Read server chunk
            ssize_t n = safe_read(sock, buffer, sizeof(buffer)-1);
            if (n == 0) { printf("Server closed connection.\n"); goto cleanup; }
            if (n < 0) { perror("read"); goto cleanup; }
            buffer[n] = '\0';
            fputs(buffer, stdout);
            fflush(stdout);

            // If server asks for input (colon or '>'), collect user line and send as string
            if (strchr(buffer, ':') || strchr(buffer, '>')) {
                // prompt user
                printf("> ");
                if (!read_user_line(line, sizeof(line))) goto cleanup;
                // send NUL-terminated string to server (so server's recv_string works)
                ssize_t s = write(sock, line, strlen(line) + 1);
                if (s <= 0) { perror("write(input)"); goto cleanup; }
                // continue reading server to see next prompt or return to main menu
                continue;
            }

            // If server returned the main menu again ("Enter your choice"), break to outer loop to read next numeric choice
            if (strstr(buffer, PROMPT_TOKEN) || strstr(buffer, "Enter your choice")) {
                break;
            }

            // Otherwise keep reading until we hit a prompt or menu token
        }
        // loop back to outer to prompt for numeric menu choice
    }

cleanup:
    close(sock);
    return 0;
} */



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

        // Receive and display initial greeting (and possibly menu)
        ssize_t bytes = read(sock, buffer, sizeof(buffer));
        if (bytes <= 0) {
            printf("Server closed the connection.\n");
            break;
        }
        printf("%s", buffer);

        // Try reading again in case the login menu arrived right after greeting
        memset(buffer, 0, sizeof(buffer));
        bytes = read(sock, buffer, sizeof(buffer));
        if (bytes > 0) {
            printf("%s", buffer);
        }

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
