// loan.h
#ifndef LOAN_H
#define LOAN_H

#include "common.h"

// ---- Loan Operations ----

// Customer: Apply for a new loan
int apply_for_loan(const char *cust_username, float amount);

// Employee: Approve or reject loan (already implemented in employee.c)
// int decide_loan_status(int loan_id, const char *emp_user, const char *decision);

// Manager: Assign a loan to employee (already in manager.c)
// int assign_loan_to_staff(int loan_id, const char *employee_user);

// View all loans (for testing or reports)
int view_all_loans_system();

// Generate next loan ID (internally used)
int get_next_loan_id();

// Utility: Check if user has any pending loan
int has_pending_loan(const char *cust_username);

#endif // LOAN_H
