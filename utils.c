// utils.c
#include"utils.h"
#include<fcntl.h>
#include<unistd.h>
#include<string.h>
#include<errno.h>
#include<stdio.h>

int read_line(int fd, char *buf, int size) {
    int i = 0;
    char c;
    while (i < size - 1 && read(fd, &c, 1) == 1) {
        if (c == '\n' || c == '\0') break;
        buf[i++] = c;
    }
    buf[i] = '\0';
    return (i > 0);
}

int lock_file(int fd, int lock_type) {
    struct flock lock;
    lock.l_type = lock_type;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;
    lock.l_pid = getpid();

    return fcntl(fd, F_SETLKW, &lock);
}

void unlock_file(int fd) {
    struct flock lock;
    lock.l_type = F_UNLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;
    fcntl(fd, F_SETLK, &lock);
}

// Authenticate user using binary .dat credentials
int authenticate_user(const char *filename, const char *username, const char *password) {
    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        perror("open failed");
        return -1;
    }

    struct Credential rec;
    int success = 0;

    while (read(fd, &rec, sizeof(rec)) == sizeof(rec)) {
        if (strcmp(rec.username, username) == 0 && strcmp(rec.password, password) == 0) {
            success = 1;
            break;
        }
    }

    close(fd);
    return success;
}

// Change password for any role
int change_password_user(const char *filename, const char *username, const char *newpass) {
    int fd = open(filename, O_RDWR);
    if (fd == -1) {
        perror("open");
        return -1;
    }

    if (lock_file(fd, F_WRLCK) == -1) return -1;

    struct Credential rec;
    off_t pos;
    int updated = 0;

    while ((pos = lseek(fd, 0, SEEK_CUR)) >= 0 && read(fd, &rec, sizeof(rec)) == sizeof(rec)) {
        if (strcmp(rec.username, username) == 0) {
            strcpy(rec.password, newpass);
            lseek(fd, pos, SEEK_SET);
            write(fd, &rec, sizeof(rec));
            updated = 1;
            break;
        }
    }

    unlock_file(fd);
    close(fd);
    return updated ? 0 : -1;
}
