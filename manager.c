// manager.c
#include "manager.h"
#include "utils.h"
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <sys/socket.h>

#define LOAN_FILE "data/loans.dat"
#define CUSTOMER_FILE "data/customers.dat"
#define FEEDBACK_FILE "feedbacks.dat"

// Assign loan application processes to employee
int assign_loan_to_staff(int loan_id, const char *employee_user) {
    int fd = open(LOAN_FILE, O_RDWR);
    if (fd == -1) {
        perror("assign_loan_to_staff: open");
        return -1;
    }

    if (lock_file(fd, F_WRLCK) == -1) {
        close(fd);
        return -1;
    }

    struct Loan record;
    off_t pos;
    int done = 0;

    while ((pos = lseek(fd, 0, SEEK_CUR)) >= 0 && read(fd, &record, sizeof(record)) == sizeof(record)) {
        if (record.loan_id == loan_id && strlen(record.assigned_to) == 0) {
            strncpy(record.assigned_to, employee_user, sizeof(record.assigned_to));
            strncpy(record.status, "assigned", sizeof(record.status));
            lseek(fd, pos, SEEK_SET);
            write(fd, &record, sizeof(record));
            done = 1;
            break;
        }
    }

    unlock_file(fd);
    close(fd);
    return done ? 0 : -1;
}

// List all unassigned loans
int list_unassigned_loans(int client_fd) {
    int fd = open(LOAN_FILE, O_RDONLY);
    if (fd == -1) {
        const char *msg = "No loan records found.\n";
        write(client_fd, msg, strlen(msg));
        return -1;
    }

    struct Loan record;
    char buffer[256];
    int count = 0;

    while (read(fd, &record, sizeof(record)) == sizeof(record)) {
        if (strlen(record.assigned_to) == 0) {
            int len = snprintf(buffer, sizeof(buffer),
                               "LoanID: %d | Applicant: %s | Amount: %.2f | Type: %s | Tenure: %d | Status: %s\n",
                               record.loan_id, record.username, record.amount,
                               record.type, record.tenure, record.status);
            write(client_fd, buffer, len);
            count++;
        }
    }

    if (count == 0)
        write(client_fd, "All loans are currently assigned.\n", 35);

    close(fd);
    return 0;
}

// Review Customer feedbacks
int review_customer_feedbacks(int client_fd) {
    int fd = open(FEEDBACK_FILE, O_RDONLY);
    if (fd == -1) {
        write(client_fd, "No feedback records found.\n", 28);
        return -1;
    }

    char lines[50][256];
    char ch;
    int count = 0, idx = 0;
    char line[256] = {0};

    while (read(fd, &ch, 1) == 1) {
        if (ch == '\n' || idx >= 255) {
            line[idx] = '\0';
            if (strlen(line) > 0)
                strncpy(lines[count++], line, sizeof(line));
            idx = 0;
            memset(line, 0, sizeof(line));
            if (count >= 50) break;
        } else {
            line[idx++] = ch;
        }
    }
    close(fd);

    int start = (count > 5) ? count - 5 : 0;
    for (int i = start; i < count; i++) {
        write(client_fd, lines[i], strlen(lines[i]));
        write(client_fd, "\n", 1);
    }

    if (count == 0)
        write(client_fd, "No feedback available.\n", 24);

    return 0;
}

// Activate/deactivate customer account
int set_customer_active_status(const char *username, int active_flag) {
    int fd = open(CUSTOMER_FILE, O_RDWR);
    if (fd == -1) {
        perror("set_customer_active_status: open");
        return -1;
    }

    if (lock_file(fd, F_WRLCK) == -1) {
        close(fd);
        return -1;
    }

    struct Customer cust;
    off_t pos;
    int modified = 0;

    while ((pos = lseek(fd, 0, SEEK_CUR)) >= 0 && read(fd, &cust, sizeof(cust)) == sizeof(cust)) {
        if (strcmp(cust.username, username) == 0) {
            cust.loan_status = active_flag; // using loan_status as 'active_flag' marker
            lseek(fd, pos, SEEK_SET);
            write(fd, &cust, sizeof(cust));
            modified = 1;
            break;
        }
    }

    unlock_file(fd);
    close(fd);
    return modified ? 0 : -1;
}

// View all loans summary
int view_all_loans(int client_fd) {
    int fd = open(LOAN_FILE, O_RDONLY);
    if (fd == -1) {
        write(client_fd, "No loans recorded.\n", 20);
        return -1;
    }

    struct Loan loan;
    char buffer[256];
    int count = 0;

    while (read(fd, &loan, sizeof(loan)) == sizeof(loan)) {
        int len = snprintf(buffer, sizeof(buffer),
                           "LoanID: %d | Applicant: %s | AssignedTo: %s | Amount: %.2f | Status: %s\n",
                           loan.loan_id, loan.username, 
                           (strlen(loan.assigned_to) > 0 ? loan.assigned_to : "Unassigned"),
                           loan.amount, loan.status);
        write(client_fd, buffer, len);
        count++;
    }

    if (count == 0)
        write(client_fd, "No loan entries found.\n", 24);

    close(fd);
    return 0;
}
