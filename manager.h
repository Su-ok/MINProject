// manager.h
#ifndef MANAGER_H
#define MANAGER_H

#include "common.h"

// ---- Manager Operations ----

// Assign a loan to an employee
int assign_loan_to_staff(int loan_id, const char *employee_user);

// View all unassigned loan requests
int list_unassigned_loans(int client_fd);

// Review all customer feedback entries (last few)
int review_customer_feedbacks(int client_fd);

// Activate or deactivate a customer account
int set_customer_active_status(const char *username, int active_flag);

// View all loans with their statuses
int view_all_loans(int client_fd);

#endif
