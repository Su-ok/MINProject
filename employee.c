// employee.c
#include "employee.h"
#include "utils.h"
#include "customer.h"
#include "loan.h"

#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <sys/socket.h>

#define CUSTOMER_FILE "data/customers.dat"
#define LOAN_FILE "data/loans.dat"

// Add new customer
int add_new_customer(const struct Customer *cust) {
    int fd = open(CUSTOMER_FILE, O_WRONLY | O_APPEND | O_CREAT, 0644);
    if (fd == -1) {
        perror("add_new_customer: open");
        return -1;
    }

    if (lock_file(fd, F_WRLCK) == -1) {
        close(fd);
        return -1;
    }

    write(fd, cust, sizeof(*cust));

    unlock_file(fd);
    close(fd);
    return 0;
}

// Modify existing customer
int update_customer_record(const char *username, const char *field, const char *new_value) {
    int fd = open(CUSTOMER_FILE, O_RDWR);
    if (fd == -1) {
        perror("update_customer_record: open");
        return -1;
    }

    if (lock_file(fd, F_WRLCK) == -1) {
        close(fd);
        return -1;
    }

    struct Customer cust;
    off_t pos;
    int updated = 0;

    while ((pos = lseek(fd, 0, SEEK_CUR)) >= 0 && read(fd, &cust, sizeof(cust)) == sizeof(cust)) {
        if (strcmp(cust.username, username) == 0) {
            if (strcmp(field, "name") == 0)
                strncpy(cust.name, new_value, sizeof(cust.name));
            else if (strcmp(field, "phone") == 0)
                strncpy(cust.phone_no, new_value, sizeof(cust.phone_no));
            else if (strcmp(field, "account") == 0)
                strncpy(cust.account_no, new_value, sizeof(cust.account_no));
            else
                break;

            lseek(fd, pos, SEEK_SET);
            write(fd, &cust, sizeof(cust));
            updated = 1;
            break;
        }
    }

    unlock_file(fd);
    close(fd);
    return updated ? 0 : -1;
}

// Approve/Reject a loan
int decide_loan_status(int loan_id, const char *emp_user, const char *decision) {
    int fd = open(LOAN_FILE, O_RDWR);
    if (fd == -1) {
        perror("decide_loan_status: open");
        return -1;
    }

    if (lock_file(fd, F_WRLCK) == -1) {
        close(fd);
        return -1;
    }

    struct Loan loan;
    off_t pos;
    int success = 0;

    while ((pos = lseek(fd, 0, SEEK_CUR)) >= 0 && read(fd, &loan, sizeof(loan)) == sizeof(loan)) {
        if (loan.loan_id == loan_id && strcmp(loan.assigned_to, emp_user) == 0) {
            strncpy(loan.status, decision, sizeof(loan.status));
            lseek(fd, pos, SEEK_SET);
            write(fd, &loan, sizeof(loan));
            success = 1;
            break;
        }
    }

    unlock_file(fd);
    close(fd);
    return success ? 0 : -1;
}

// View assigned loan appplications
int display_assigned_loans(int client_fd, const char *emp_user) {
    int fd = open(LOAN_FILE, O_RDONLY);
    if (fd == -1) {
        const char *msg = "No loans available.\n";
        write(client_fd, msg, strlen(msg));
        return -1;
    }

    struct Loan loan;
    char buffer[256];
    int count = 0;

    while (read(fd, &loan, sizeof(loan)) == sizeof(loan)) {
        if (strcmp(loan.assigned_to, emp_user) == 0) {
            int len = snprintf(buffer, sizeof(buffer),
                               "LoanID: %d | User: %s | Amount: %.2f | Type: %s | Tenure: %d | Status: %s\n",
                               loan.loan_id, loan.username, loan.amount, loan.type,
                               loan.tenure, loan.status);
            write(client_fd, buffer, len);
            count++;
        }
    }

    if (count == 0)
        write(client_fd, "No assigned loans found.\n", 25);

    close(fd);
    return 0;
}

// View transaction history of customer
int show_customer_transactions(int client_fd, const char *cust_username) {
    char txn_file[100];
    snprintf(txn_file, sizeof(txn_file), "data/%s_txn.dat", cust_username);
    int fd = open(txn_file, O_RDONLY);
    if (fd == -1) {
        const char *msg = "No transaction history available for this customer.\n";
        write(client_fd, msg, strlen(msg));
        return -1;
    }

    struct Transaction txn;
    char buffer[256];
    int count = 0;

    while (read(fd, &txn, sizeof(txn)) == sizeof(txn)) {
        int len = snprintf(buffer, sizeof(buffer),
                           "TxnID: %s | From: %s | To: %s | Amount: %.2f | Type: %s | Time: %s\n",
                           txn.txn_id, txn.sender, txn.receiver, txn.amount, txn.txn_type, txn.timestamp);
        write(client_fd, buffer, len);
        count++;
    }

    if (count == 0)
        write(client_fd, "No transactions recorded.\n", 27);

    close(fd);
    return 0;
}
