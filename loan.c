#include"loan.h"
#include"utils.h"

#include<fcntl.h>
#include<unistd.h>
#include<string.h>
#include<stdio.h>
#include<stdlib.h>
#include<time.h>

#define LOAN_FILE "data/loans.dat"

// ---- Generate unique loan ID ----
int get_next_loan_id() {
    int fd = open(LOAN_FILE, O_RDONLY | O_CREAT, 0644);
    if (fd == -1) {
        perror("get_next_loan_id: open");
        return 1;
    }

    struct Loan loan;
    int max_id = 0;

    while (read(fd, &loan, sizeof(loan)) == sizeof(loan)) {
        if (loan.loan_id > max_id)
            max_id = loan.loan_id;
    }

    close(fd);
    return max_id + 1;
}

// ---- Check if a user has a pending loan ----
int has_pending_loan(const char *cust_username) {
    int fd = open(LOAN_FILE, O_RDONLY);
    if (fd == -1) return 0;

    struct Loan loan;
    while (read(fd, &loan, sizeof(loan)) == sizeof(loan)) {
        if (strcmp(loan.username, cust_username) == 0 && strcmp(loan.status, "pending") == 0) {
            close(fd);
            return 1;
        }
    }

    close(fd);
    return 0;
}

// ---- Apply for a new loan ----
int apply_for_loan(const char *cust_username, float amount) {
    int fd = open(LOAN_FILE, O_RDWR | O_APPEND | O_CREAT, 0644);
    if (fd == -1) {
        perror("apply_for_loan: open");
        return -1;
    }

    if (lock_file(fd, F_WRLCK) == -1) {
        close(fd);
        return -1;
    }

    // Prevent duplicate pending loans
    if (has_pending_loan(cust_username)) {
        unlock_file(fd);
        close(fd);
        return -2;  // already pending
    }

    struct Loan loan;
    memset(&loan, 0, sizeof(loan));

    loan.loan_id = get_next_loan_id();
    strncpy(loan.username, cust_username, sizeof(loan.username) - 1);
    loan.amount = amount;

    // Choose random loan type for demonstration (or prompt in client)
    const char *types[] = {"Personal", "Home", "Education", "Vehicle"};
    srand(time(NULL));
    strncpy(loan.type, types[rand() % 4], sizeof(loan.type) - 1);

    // Set tenure based on loan type (example mapping)
    if (strcmp(loan.type, "Home") == 0)
        loan.tenure = 240;  // 20 years
    else if (strcmp(loan.type, "Vehicle") == 0)
        loan.tenure = 60;   // 5 years
    else
        loan.tenure = 36;   // 3 years default

    strncpy(loan.status, "pending", sizeof(loan.status) - 1);
    loan.interest_rate = 8.5; // flat rate demo
    strncpy(loan.assigned_to, "", sizeof(loan.assigned_to)); // not assigned yet

    ssize_t w = write(fd, &loan, sizeof(loan));

    unlock_file(fd);
    close(fd);

    return (w == sizeof(loan)) ? 0 : -1;
}

// ---- View all loans (system report, for debug/testing) ----
int view_all_loans_system() {
    int fd = open(LOAN_FILE, O_RDONLY);
    if (fd == -1) {
        perror("view_all_loans_system: open");
        return -1;
    }

    struct Loan loan;
    printf("\n===== LOAN RECORDS =====\n");
    while (read(fd, &loan, sizeof(loan)) == sizeof(loan)) {
        printf("LoanID: %d | User: %s | Amount: %.2f | Type: %s | Tenure: %d | Rate: %.2f | Status: %s | AssignedTo: %s\n",
               loan.loan_id, loan.username, loan.amount, loan.type,
               loan.tenure, loan.interest_rate, loan.status,
               strlen(loan.assigned_to) ? loan.assigned_to : "None");
    }
    printf("=========================\n");

    close(fd);
    return 0;
}
