// common.h
#ifndef COMMON_H
#define COMMON_H

#define MAX_NAME 50
#define MAX_PASS 20
#define MAX_PHONE 15
#define MAX_ROLE 20

struct Customer {
    char username[MAX_NAME];
    char account_no[20];
    char name[MAX_NAME];
    char phone_no[MAX_PHONE];
    float balance;
    int loan_status;
    float loan_amount;
};

struct Employee {
    char username[MAX_NAME];
    char name[MAX_NAME];
    char role[MAX_ROLE];  // "Manager" or "Staff"
    char phone[MAX_PHONE];
    float salary;
};

struct Loan {
    int loan_id;
    char username[MAX_NAME];
    char assigned_to[MAX_NAME];  // employee username
    float amount;
    char type[20];
    int tenure;
    float interest_rate;
    char status[20]; // "pending", "approved", "rejected"
};

#endif
