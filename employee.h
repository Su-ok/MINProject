// employee.h
#ifndef EMPLOYEE_H
#define EMPLOYEE_H

#include "common.h"

// ---- Employee Operations ----

// Add new customer account (to customers.dat)
int add_new_customer(const struct Customer *cust);

// Update specific customer detail (phone, name, etc.)
int update_customer_record(const char *username, const char *field, const char *new_value);

// Approve or reject a loan
int decide_loan_status(int loan_id, const char *emp_user, const char *decision);

// Display loans assigned to this employee
int display_assigned_loans(int client_fd, const char *emp_user);

// View a customer's transaction history
int show_customer_transactions(int client_fd, const char *cust_username);

#endif
