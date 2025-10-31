// admin.h
#ifndef ADMIN_H
#define ADMIN_H

#include "common.h"

// ---- Admin operations ----

// Create a new employee record (append to data/employees.dat)
// Returns 0 on success, -1 on error
int create_employee_record(const struct Employee *emp);

// Update a field of an existing employee record
// field_name examples: "name", "role", "phone", "salary"
// Returns 0 on success, 1 if not found, -1 on error, 2 if unknown field
int modify_employee_field(const char *employee_user, const char *field_name, const char *new_value);

// Promote employee to manager or demote manager to staff
// action: "promote" or "demote"
// Returns 0 on success, 1 if not found, -1 on error
int change_employee_role(const char *employee_user, const char *action);

// Change password for any employee/admin using utils helper
// creds_file should be the credential dat file path (e.g., "data/employee_creds.dat")
// Returns 0 on success, -1 on error
int admin_change_password(const char *creds_file, const char *employee_user, const char *new_password);

// List all employees to a client socket (writes textual list)
// Returns number of records written, or -1 on error
int list_all_employees(int client_fd);

#endif // ADMIN_H
