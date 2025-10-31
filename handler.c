#include"handler.h"
#include"utils.h"
#include"customer.h"
#include"employee.h"
#include"manager.h"
#include"admin.h"

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>

char whoami[MAX_NAME];
char shared_usernames[MAX_USERS][MAX_NAME];

// Main client handler (runs in thread)
void *handle_client(void *socket_desc) {
    int sock = *(int *)socket_desc;
    free(socket_desc);

    int choice;
    const char *menu =
        "\n=== LOGIN MENU ===\n"
        "1. Customer\n"
        "2. Employee\n"
        "3. Manager\n"
        "4. Admin\n"
        "5. Exit\n"
        "Enter your choice: ";

    while (1) {
        write(sock, menu, strlen(menu));
        char choice_str[16];
        if (recv_string(sock, choice_str, sizeof(choice_str)) <= 0)
            break;
        choice=atoi(choice_str);
        switch (choice) {
            case 1:
                handle_customer(sock);
                break;
            case 2:
                handle_employee(sock);
                break;
            case 3:
                handle_manager(sock);
                break;
            case 4:
                handle_admin(sock);
                break;
            case 5:
                write(sock, "Goodbye!\n", 9);
                close(sock);
                return NULL;
            default:
                write(sock, "Invalid choice.\n", 16);
                break;
        }
    }

    close(sock);
    return NULL;
}

static void send_text(int sock, const char *txt) {
    if (!txt) return;
    write(sock, txt, strlen(txt));
}

static int recv_string(int sock, char *buf, size_t bufsize) {
    ssize_t r = read(sock, buf, bufsize - 1);
    if (r <= 0) { buf[0] = '\0'; return -1; }
    // ensure null termination; client should send NUL-terminated or newline; trim newline
    buf[r] = '\0';
    char *p = strchr(buf, '\n'); if (p) *p = '\0';
    p = strchr(buf, '\r'); if (p) *p = '\0';
    return 0;
}

void handle_customer(int sock) {
    char sendbuf[512];
    char uname[MAX_NAME], pwd[MAX_PASS];

    send_text(sock, "\n-- CUSTOMER LOGIN --\nUsername: ");
    if (recv_string(sock, uname, sizeof(uname)) < 0) return;

    send_text(sock, "Password: ");
    if (recv_string(sock, pwd, sizeof(pwd)) < 0) return;

    // authenticate_user expects a credential file path
    if (authenticate_user("data/customer_creds.dat", uname, pwd) != 1) {
        send_text(sock, "ERR: invalid credentials\n");
        return;
    }

    // register session username in whoami and shared_usernames
    strncpy(whoami, uname, sizeof(whoami)-1);
    for (int i = 0; i < MAX_USERS; ++i) {
        if (shared_usernames[i][0] == '\0') { strncpy(shared_usernames[i], whoami, MAX_NAME-1); break; }
    }

    send_text(sock, "OK: login successful\n");

    while (1) {
        snprintf(sendbuf, sizeof(sendbuf),
            "\nCustomer Menu:\n"
            "1. View Balance\n"
            "2. Deposit\n"
            "3. Withdraw\n"
            "4. Transfer\n"
            "5. Apply Loan\n"
            "6. View Transactions\n"
            "7. Submit Feedback\n"
            "8. Logout\n"
            "Enter choice: ");
        send_text(sock, sendbuf);

        char choice_str[16];
        if (recv_string(sock, choice_str, sizeof(choice_str)) <= 0) break;
        int choice = atoi(choice_str);

        if (choice == 1) {
            float bal = fetch_account_balance(whoami);
            char out[128]; snprintf(out, sizeof(out), "Balance: %.2f\n", bal);
            send_text(sock, out);
        }
        else if (choice == 2 || choice == 3) { // deposit or withdraw
            if (choice == 2) send_text(sock, "Enter amount to deposit: ");
            else send_text(sock, "Enter amount to withdraw: ");
            float amt = 0.0f;
            if (read(sock, &amt, sizeof(amt)) <= 0) break;
            float delta = (choice == 2) ? amt : -amt;
            int r = modify_balance(whoami, delta);
            if (r == 0) send_text(sock, "OK: operation successful\n");
            else send_text(sock, "ERR: operation failed\n");
        }
        else if (choice == 4) {
            send_text(sock, "Enter recipient username: ");
            char to_user[MAX_NAME];
            if (recv_string(sock, to_user, sizeof(to_user)) < 0) break;
            send_text(sock, "Enter amount: ");
            float amt = 0.0f; if (read(sock, &amt, sizeof(amt)) <= 0) break;
            int r = execute_transfer(whoami, to_user, amt);
            if (r == 0) send_text(sock, "OK: transfer successful\n");
            else send_text(sock, "ERR: transfer failed\n");
        }
        else if (choice == 5) {
            // For brevity assume apply_loan exists in loan module
            send_text(sock, "Enter loan amount: ");
            float lam = 0.0f; if (read(sock, &lam, sizeof(lam)) <= 0) break;
            // other details can be prompted similarly; here we call loan API
            if (apply_for_loan(whoami, lam) == 0) send_text(sock, "OK: loan request submitted\n");
            else send_text(sock, "ERR: loan submission failed\n");
        }
        else if (choice == 6) {
            show_transaction_history(sock, whoami); // streams transactions to client
        }
        else if (choice == 7) {
            send_text(sock, "Enter feedback (single line): ");
            char feedback[256];
            if (recv_string(sock, feedback, sizeof(feedback)) < 0) break;
            if (save_feedback(whoami, feedback) == 0) send_text(sock, "OK: feedback saved\n");
            else send_text(sock, "ERR: feedback failed\n");
        }
        else if (choice == 8) {
            // remove from shared_usernames
            for (int i = 0; i < MAX_USERS; ++i) {
                if (strcmp(shared_usernames[i], whoami) == 0) shared_usernames[i][0] = '\0';
            }
            whoami[0] = '\0';
            send_text(sock, "OK: logged out\n");
            break;
        }
        else {
            send_text(sock, "Invalid option\n");
        }
    }
}

// ---------- EMPLOYEE handler ----------
void handle_employee(int sock) {
    char uname[MAX_NAME], pwd[MAX_PASS];
    send_text(sock, "\n-- EMPLOYEE LOGIN --\nUsername: ");
    if (recv_string(sock, uname, sizeof(uname)) < 0) return;
    send_text(sock, "Password: ");
    if (recv_string(sock, pwd, sizeof(pwd)) < 0) return;

    if (authenticate_user("data/employee_creds.dat", uname, pwd) != 1) {
        send_text(sock, "ERR: invalid credentials\n"); return;
    }
    strncpy(whoami, uname, sizeof(whoami)-1);
    for (int i = 0; i < MAX_USERS; ++i) if (shared_usernames[i][0] == '\0') { strncpy(shared_usernames[i], whoami, MAX_NAME-1); break; }

    send_text(sock, "OK: employee login successful\n");

    while (1) {
        send_text(sock,
            "\nEmployee Menu:\n"
            "1. Add Customer\n"
            "2. Modify Customer\n"
            "3. Approve/Reject Loan\n"
            "4. View Assigned Loans\n"
            "5. View Customer Transactions\n"
            "6. Change Password\n"
            "7. Logout\n"
            "Enter choice: ");
        char choice_str[16];
        if (recv_string(sock, choice_str, sizeof(choice_str)) <= 0) break;
        int choice = atoi(choice_str);

        if (choice == 1) {
            // receive customer struct fields (simplified)
            struct Customer nc;
            memset(&nc, 0, sizeof(nc));
            send_text(sock, "Enter username: "); if (recv_string(sock, nc.username, sizeof(nc.username)) < 0) break;
            send_text(sock, "Enter account number: "); if (recv_string(sock, nc.account_no, sizeof(nc.account_no)) < 0) break;
            send_text(sock, "Enter name: "); if (recv_string(sock, nc.name, sizeof(nc.name)) < 0) break;
            send_text(sock, "Enter phone: "); if (recv_string(sock, nc.phone_no, sizeof(nc.phone_no)) < 0) break;
            send_text(sock, "Enter initial balance (float): "); float bal=0; if (read(sock, &bal, sizeof(bal)) <= 0) break; nc.balance = bal;
            nc.loan_status = 0; nc.loan_amount = 0.0f;
            if (add_new_customer(&nc) == 0) send_text(sock, "OK: customer added\n"); else send_text(sock, "ERR: add customer failed\n");
        }
        else if (choice == 2) {
            send_text(sock, "Enter customer username to modify: "); char target[MAX_NAME]; if (recv_string(sock, target, sizeof(target)) < 0) break;
            send_text(sock, "Enter field (name/phone/account): "); char field[40]; if (recv_string(sock, field, sizeof(field)) < 0) break;
            send_text(sock, "Enter new value: "); char val[100]; if (recv_string(sock, val, sizeof(val)) < 0) break;
            if (update_customer_record(target, field, val) == 0) send_text(sock, "OK: update success\n"); else send_text(sock, "ERR: update failed\n");
        }
        else if (choice == 3) {
            send_text(sock, "Enter loan id: "); int lid; if (read(sock, &lid, sizeof(lid)) <= 0) break;
            send_text(sock, "Enter decision (approve/reject): "); char dec[20]; if (recv_string(sock, dec, sizeof(dec)) < 0) break;
            if (decide_loan_status(lid, whoami, dec) == 0) send_text(sock, "OK: loan processed\n"); else send_text(sock, "ERR: process failed\n");
        }
        else if (choice == 4) {
            display_assigned_loans(sock, whoami);
        }
        else if (choice == 5) {
            send_text(sock, "Enter customer username: "); char cu[MAX_NAME]; if (recv_string(sock, cu, sizeof(cu)) < 0) break;
            show_customer_transactions(sock, cu);
        }
        else if (choice == 6) {
            send_text(sock, "Enter new password: "); char npwd[MAX_PASS]; if (recv_string(sock, npwd, sizeof(npwd)) < 0) break;
            if (change_password_user("data/employee_creds.dat", whoami, npwd) == 0) send_text(sock, "OK: password changed\n"); else send_text(sock, "ERR: change failed\n");
        }
        else if (choice == 7) {
            for (int i = 0; i < MAX_USERS; ++i) if (strcmp(shared_usernames[i], whoami) == 0) shared_usernames[i][0] = '\0';
            whoami[0] = '\0';
            send_text(sock, "OK: logged out\n");
            break;
        }
        else send_text(sock, "Invalid choice\n");
    }
}

// ---------- MANAGER handler ----------
void handle_manager(int sock) {
    char uname[MAX_NAME], pwd[MAX_PASS];
    send_text(sock, "\n-- MANAGER LOGIN --\nUsername: ");
    if (recv_string(sock, uname, sizeof(uname)) < 0) return;
    send_text(sock, "Password: ");
    if (recv_string(sock, pwd, sizeof(pwd)) < 0) return;

    if (authenticate_user("data/manager_creds.dat", uname, pwd) != 1) {
        send_text(sock, "ERR: invalid credentials\n"); return;
    }
    strncpy(whoami, uname, sizeof(whoami)-1);
    for (int i = 0; i < MAX_USERS; ++i) if (shared_usernames[i][0] == '\0') { strncpy(shared_usernames[i], whoami, MAX_NAME-1); break; }

    send_text(sock, "OK: manager login successful\n");

    while (1) {
        send_text(sock,
            "\nManager Menu:\n"
            "1. List Unassigned Loans\n"
            "2. Assign Loan to Employee\n"
            "3. Review Feedback\n"
            "4. Activate/Deactivate Customer\n"
            "5. View All Loans\n"
            "6. Change Password\n"
            "7. Logout\n"
            "Enter choice: ");
        char choice_str[16];
        if (recv_string(sock, choice_str, sizeof(choice_str)) <= 0) break;
        int choice = atoi(choice_str);

        if (choice == 1) {
            list_unassigned_loans(sock);
        }
        else if (choice == 2) {
            send_text(sock, "Enter loan id: "); int lid; if (read(sock, &lid, sizeof(lid)) <= 0) break;
            send_text(sock, "Enter employee username: "); char empu[MAX_NAME]; if (recv_string(sock, empu, sizeof(empu)) < 0) break;
            if (assign_loan_to_staff(lid, empu) == 0) send_text(sock, "OK: loan assigned\n"); else send_text(sock, "ERR: assign failed\n");
        }
        else if (choice == 3) {
            review_customer_feedbacks(sock);
        }
        else if (choice == 4) {
            send_text(sock, "Enter customer username: "); char cu[MAX_NAME]; if (recv_string(sock, cu, sizeof(cu)) < 0) break;
            send_text(sock, "Enter status (1 = active, 0 = inactive): "); int flag=0; if (read(sock, &flag, sizeof(flag)) <= 0) break;
            if (set_customer_active_status(cu, flag) == 0) send_text(sock, "OK: status updated\n"); else send_text(sock, "ERR: update failed\n");
        }
        else if (choice == 5) {
            view_all_loans(sock);
        }
        else if (choice == 6) {
            send_text(sock, "Enter new password: "); char npwd[MAX_PASS]; if (recv_string(sock, npwd, sizeof(npwd)) < 0) break;
            if (change_password_user("data/manager_creds.dat", whoami, npwd) == 0) send_text(sock, "OK: password changed\n"); else send_text(sock, "ERR: change failed\n");
        }
        else if (choice == 7) {
            for (int i = 0; i < MAX_USERS; ++i) if (strcmp(shared_usernames[i], whoami) == 0) shared_usernames[i][0] = '\0';
            whoami[0] = '\0';
            send_text(sock, "OK: logged out\n");
            break;
        }
        else send_text(sock, "Invalid choice\n");
    }
}

// ---------- ADMIN handler ----------
void handle_admin(int sock) {
    char uname[MAX_NAME], pwd[MAX_PASS];
    send_text(sock, "\n-- ADMIN LOGIN --\nUsername: ");
    if (recv_string(sock, uname, sizeof(uname)) < 0) return;
    send_text(sock, "Password: ");
    if (recv_string(sock, pwd, sizeof(pwd)) < 0) return;

    if (authenticate_user("data/admin_creds.dat", uname, pwd) != 1) {
        send_text(sock, "ERR: invalid credentials\n"); return;
    }
    strncpy(whoami, uname, sizeof(whoami)-1);
    for (int i = 0; i < MAX_USERS; ++i) if (shared_usernames[i][0] == '\0') { strncpy(shared_usernames[i], whoami, MAX_NAME-1); break; }

    send_text(sock, "OK: admin login successful\n");

    while (1) {
        send_text(sock,
            "\nAdmin Menu:\n"
            "1. Add Employee\n"
            "2. Modify Employee\n"
            "3. Modify Customer\n"
            "4. Promote/Demote Employee\n"
            "5. List Employees\n"
            "6. Change Password\n"
            "7. Logout\n"
            "Enter choice: ");
        char choice_str[16];
        if (recv_string(sock, choice_str, sizeof(choice_str)) <= 0) break;
        int choice = atoi(choice_str);

        if (choice == 1) {
            struct Employee e; memset(&e,0,sizeof(e));
            send_text(sock, "Enter username: "); if (recv_string(sock, e.username, sizeof(e.username)) < 0) break;
            send_text(sock, "Enter name: "); if (recv_string(sock, e.name, sizeof(e.name)) < 0) break;
            send_text(sock, "Enter role (Manager/Staff): "); if (recv_string(sock, e.role, sizeof(e.role)) < 0) break;
            send_text(sock, "Enter phone: "); if (recv_string(sock, e.phone, sizeof(e.phone)) < 0) break;
            send_text(sock, "Enter salary (float): "); float sal=0; if (read(sock, &sal, sizeof(sal)) <= 0) break; e.salary = sal;
            if (create_employee_record(&e) == 0) send_text(sock, "OK: employee created\n"); else send_text(sock, "ERR: create failed\n");
        }
        else if (choice == 2) {
            send_text(sock, "Enter employee username: "); char empu[MAX_NAME]; if (recv_string(sock, empu, sizeof(empu)) < 0) break;
            send_text(sock, "Enter field to modify (name/role/phone/salary): "); char field[32]; if (recv_string(sock, field, sizeof(field)) < 0) break;
            send_text(sock, "Enter new value: "); char val[128]; if (recv_string(sock, val, sizeof(val)) < 0) break;
            int r = modify_employee_field(empu, field, val);
            if (r == 0) send_text(sock, "OK: modified\n");
            else if (r == 1) send_text(sock, "ERR: employee not found\n");
            else if (r == 2) send_text(sock, "ERR: unknown field\n");
            else send_text(sock, "ERR: modify failed\n");
        }
        else if (choice == 3) {
            send_text(sock, "Enter customer username: "); char cu[MAX_NAME]; if (recv_string(sock, cu, sizeof(cu)) < 0) break;
            send_text(sock, "Enter field to modify (name/phone/account): "); char field[32]; if (recv_string(sock, field, sizeof(field)) < 0) break;
            send_text(sock, "Enter new value: "); char val[128]; if (recv_string(sock, val, sizeof(val)) < 0) break;
            if (update_customer_record(cu, field, val) == 0) send_text(sock, "OK: customer modified\n"); else send_text(sock, "ERR: modify failed\n");
        }
        else if (choice == 4) {
            send_text(sock, "Enter action (promote/demote): "); char action[16]; if (recv_string(sock, action, sizeof(action)) < 0) break;
            send_text(sock, "Enter employee username: "); char empu2[MAX_NAME]; if (recv_string(sock, empu2, sizeof(empu2)) < 0) break;
            if (change_employee_role(empu2, action) == 0) send_text(sock, "OK: role changed\n"); else send_text(sock, "ERR: role change failed\n");
        }
        else if (choice == 5) {
            list_all_employees(sock);
        }
        else if (choice == 6) {
            send_text(sock, "Enter new password: "); char npwd[MAX_PASS]; if (recv_string(sock, npwd, sizeof(npwd)) < 0) break;
            if (admin_change_password("data/admin_creds.txt", whoami, npwd) == 0) send_text(sock, "OK: password changed\n"); else send_text(sock, "ERR: change failed\n");
        }
        else if (choice == 7) {
            for (int i = 0; i < MAX_USERS; ++i) if (strcmp(shared_usernames[i], whoami) == 0) shared_usernames[i][0] = '\0';
            whoami[0] = '\0';
            send_text(sock, "OK: logged out\n");
            break;
        }
        else send_text(sock, "Invalid choice\n");
    }
}