// admin.c
#define _POSIX_C_SOURCE 200809L
#include"admin.h"
#include"utils.h"    // for lock_file/unlock_file and change_password_user if available
#include<fcntl.h>
#include<unistd.h>
#include<string.h>
#include<stdio.h>
#include<sys/socket.h>
#include<stdlib.h>

#define EMPLOYEE_STORE "data/employees.dat"

// Append a new employee record
int create_employee_record(const struct Employee *emp) {
    int fd = open(EMPLOYEE_STORE, O_WRONLY | O_APPEND | O_CREAT, 0644);
    if (fd == -1) {
        perror("create_employee_record: open");
        return -1;
    }

    if (lock_file(fd, F_WRLCK) == -1) {
        close(fd);
        return -1;
    }

    ssize_t w = write(fd, emp, sizeof(*emp));
    unlock_file(fd);
    close(fd);

    return (w == (ssize_t)sizeof(*emp)) ? 0 : -1;
}

// Modify an employee field (name, role, phone, salary)
int modify_employee_field(const char *employee_user, const char *field_name, const char *new_value) {
    int fd = open(EMPLOYEE_STORE, O_RDWR);
    if (fd == -1) {
        perror("modify_employee_field: open");
        return -1;
    }

    if (lock_file(fd, F_WRLCK) == -1) {
        close(fd);
        return -1;
    }

    struct Employee rec;
    off_t pos;
    int found = 0;

    while ((pos = lseek(fd, 0, SEEK_CUR)) >= 0 && read(fd, &rec, sizeof(rec)) == sizeof(rec)) {
        if (strncmp(rec.username, employee_user, sizeof(rec.username)) == 0) {
            found = 1;

            if (strcmp(field_name, "name") == 0) {
                strncpy(rec.name, new_value, sizeof(rec.name)-1);
                rec.name[sizeof(rec.name)-1] = '\0';
            } else if (strcmp(field_name, "role") == 0) {
                strncpy(rec.role, new_value, sizeof(rec.role)-1);
                rec.role[sizeof(rec.role)-1] = '\0';
            } else if (strcmp(field_name, "phone") == 0) {
                strncpy(rec.phone, new_value, sizeof(rec.phone)-1);
                rec.phone[sizeof(rec.phone)-1] = '\0';
            } else if (strcmp(field_name, "salary") == 0) {
                // store salary as text -> convert to float and assign
                rec.salary = (float)strtod(new_value, NULL);
            } else {
                unlock_file(fd);
                close(fd);
                return 2; // unknown field
            }

            // write updated record back
            if (lseek(fd, pos, SEEK_SET) == (off_t)-1) {
                perror("modify_employee_field: lseek");
                unlock_file(fd);
                close(fd);
                return -1;
            }
            if (write(fd, &rec, sizeof(rec)) != sizeof(rec)) {
                perror("modify_employee_field: write");
                unlock_file(fd);
                close(fd);
                return -1;
            }
            break;
        }
    }

    unlock_file(fd);
    close(fd);
    return found ? 0 : 1;
}

// Promote or demote an employee's role
int change_employee_role(const char *employee_user, const char *action) {
    if (!employee_user || !action) return -1;
    int fd = open(EMPLOYEE_STORE, O_RDWR);
    if (fd == -1) {
        perror("change_employee_role: open");
        return -1;
    }

    if (lock_file(fd, F_WRLCK) == -1) {
        close(fd);
        return -1;
    }

    struct Employee rec;
    off_t pos;
    int result = 1; // not found by default

    while ((pos = lseek(fd, 0, SEEK_CUR)) >= 0 && read(fd, &rec, sizeof(rec)) == sizeof(rec)) {
        if (strncmp(rec.username, employee_user, sizeof(rec.username)) == 0) {
            if (strcmp(action, "promote") == 0) {
                strncpy(rec.role, "Manager", sizeof(rec.role)-1);
                rec.role[sizeof(rec.role)-1] = '\0';
            } else if (strcmp(action, "demote") == 0) {
                strncpy(rec.role, "Staff", sizeof(rec.role)-1);
                rec.role[sizeof(rec.role)-1] = '\0';
            } else {
                unlock_file(fd);
                close(fd);
                return -1; // invalid action
            }

            if (lseek(fd, pos, SEEK_SET) == (off_t)-1) { perror("change_employee_role: lseek"); unlock_file(fd); close(fd); return -1; }
            if (write(fd, &rec, sizeof(rec)) != sizeof(rec)) { perror("change_employee_role: write"); unlock_file(fd); close(fd); return -1; }
            result = 0; // success
            break;
        }
    }

    unlock_file(fd);
    close(fd);
    return result;
}

// Use utils helper to change password in a credentials file
int admin_change_password(const char *creds_file, const char *employee_user, const char *new_password) {
    // repurpose change_password_user() from utils.h if it matches signature:
    // int change_password_user(const char *filename, const char *username, const char *newpass);
#ifdef HAVE_CHANGE_PASS_UTIL
    if (change_password_user(creds_file, employee_user, new_password) == 0) return 0;
    return -1;
#else
    // If helper unavailable, implement a small system-call based password update for creds_file
    int fd = open(creds_file, O_RDWR);
    if (fd == -1) {
        perror("admin_change_password: open");
        return -1;
    }
    if (lock_file(fd, F_WRLCK) == -1) { close(fd); return -1; }

    // Read entries into memory (simple approach)
    off_t file_size = lseek(fd, 0, SEEK_END);
    if (file_size == (off_t)-1) { unlock_file(fd); close(fd); return -1; }
    lseek(fd, 0, SEEK_SET);

    char *buf = malloc(file_size + 1);
    if (!buf) { unlock_file(fd); close(fd); return -1; }
    ssize_t r = read(fd, buf, file_size);
    buf[file_size] = '\0';

    // simple newline-separated username:password file format assumed
    char *saveptr = NULL;
    char *line = strtok_r(buf, "\n", &saveptr);
    int modified = 0;
    off_t write_pos = 0;

    // create a temporary buffer for new content
    char *outbuf = malloc(file_size + 256);
    if (!outbuf) { free(buf); unlock_file(fd); close(fd); return -1; }
    outbuf[0] = '\0';

    while (line) {
        char uname[MAX_NAME], upass[128];
        if (sscanf(line, "%49[^:]:%127s", uname, upass) == 2) {
            if (strcmp(uname, employee_user) == 0) {
                // replace password
                char tmp[200];
                snprintf(tmp, sizeof(tmp), "%s:%s\n", uname, new_password);
                strcat(outbuf, tmp);
                modified = 1;
            } else {
                strcat(outbuf, line);
                strcat(outbuf, "\n");
            }
        } else {
            strcat(outbuf, line);
            strcat(outbuf, "\n");
        }
        line = strtok_r(NULL, "\n", &saveptr);
    }

    // write back
    lseek(fd, 0, SEEK_SET);
    ftruncate(fd, 0);
    ssize_t w = write(fd, outbuf, strlen(outbuf));

    free(buf);
    free(outbuf);

    unlock_file(fd);
    close(fd);
    return (modified && w == (ssize_t)strlen(outbuf)) ? 0 : -1;
#endif
}

// Send a textual list of all employees to a connected client
int list_all_employees(int client_fd) {
    int fd = open(EMPLOYEE_STORE, O_RDONLY);
    if (fd == -1) {
        const char *msg = "No employee records found.\n";
        write(client_fd, msg, strlen(msg));
        return -1;
    }

    struct Employee rec;
    char buffer[256];
    int count = 0;

    while (read(fd, &rec, sizeof(rec)) == sizeof(rec)) {
        int len = snprintf(buffer, sizeof(buffer),
                           "User: %s | Name: %s | Role: %s | Phone: %s | Salary: %.2f\n",
                           rec.username, rec.name, rec.role, rec.phone, rec.salary);
        write(client_fd, buffer, len);
        count++;
    }

    if (count == 0) {
        write(client_fd, "No employees in store.\n", 23);
    }

    close(fd);
    return count;
}
