#ifndef UTILS_H
#define UTILS_H

#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#define MAX_NAME 50
#define MAX_PASS 20
#define MAX_USERS 10

//credentials stored in binary files
struct Credential {
    char username[MAX_NAME];
    char password[MAX_PASS];
};

// Shared globals (track logged-in sessions)
extern char whoami[MAX_NAME];
extern char shared_usernames[MAX_USERS][MAX_NAME];

// --- Core utility functions ---

// Reads a single line from an fd
int read_line(int fd, char *buf, int size);

// Locks an entire file (F_RDLCK or F_WRLCK)
int lock_file(int fd, int lock_type);

// Unlocks an entire file
void unlock_file(int fd);

// Authenticate user from a credentials file
int authenticate_user(const char *filename, const char *username, const char *password);

// Change password for any role
int change_password_user(const char *filename, const char *username, const char *newpass);

#endif
