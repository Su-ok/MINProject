// customer.c
#include"customer.h"
#include"utils.h"
#include<fcntl.h>
#include<string.h>
#include<stdio.h>
#include<time.h>
#include<unistd.h>
#include<sys/socket.h>

#define MAX_TXNS 100
#define FEEDBACK_FILE "feedbacks.dat"

// Utility: generate timestamp
static void make_timestamp(char *buf, size_t len) {
    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);
    strftime(buf, len, "%Y-%m-%d %H:%M:%S", tm_info);
}

//Record transaction into .dat
void record_transaction(const char *user, const char *sender, const char *receiver, float amt, const char *type) {
    char txn_file[100];
    snprintf(txn_file, sizeof(txn_file), "data/%s_txn.dat", user);

    int fd = open(txn_file, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd == -1) {
        perror("record_transaction: open failed");
        return;
    }

    if (lock_file(fd, F_WRLCK) == -1) {
        close(fd);
        return;
    }

    struct Transaction t;
    memset(&t, 0, sizeof(t));
    snprintf(t.txn_id, sizeof(t.txn_id), "%ld", time(NULL));  // unique-ish id
    strncpy(t.sender, sender, MAX_NAME);
    strncpy(t.receiver, receiver, MAX_NAME);
    t.amount = amt;
    strncpy(t.txn_type, type, sizeof(t.txn_type));
    make_timestamp(t.timestamp, sizeof(t.timestamp));

    write(fd, &t, sizeof(t));

    unlock_file(fd);
    close(fd);
}

// Check balance in customers.dat
float fetch_account_balance(const char *user_id) {
    int fd = open("data/customers.dat", O_RDONLY);
    if (fd == -1) {
        perror("fetch_account_balance: open");
        return -1;
    }

    if (lock_file(fd, F_RDLCK) == -1) {
        close(fd);
        return -1;
    }

    struct Customer cust;
    float balance = -1;

    while (read(fd, &cust, sizeof(cust)) == sizeof(cust)) {
        if (strcmp(cust.username, user_id) == 0) {
            balance = cust.balance;
            break;
        }
    }

    unlock_file(fd);
    close(fd);
    return balance;
}

// Deposit/Withdraw
int modify_balance(const char *user_id, float delta) {
    int fd = open("data/customers.dat", O_RDWR);
    if (fd == -1) {
        perror("modify_balance: open");
        return -1;
    }

    if (lock_file(fd, F_WRLCK) == -1) {
        close(fd);
        return -1;
    }

    struct Customer cust;
    off_t pos;
    int success = 0;

    while ((pos = lseek(fd, 0, SEEK_CUR)) >= 0 && read(fd, &cust, sizeof(cust)) == sizeof(cust)) {
        if (strcmp(cust.username, user_id) == 0) {
            if (cust.balance + delta < 0) break; // insufficient funds
            cust.balance += delta;
            lseek(fd, pos, SEEK_SET);
            write(fd, &cust, sizeof(cust));
            record_transaction(user_id, "self", "self", delta, (delta > 0) ? "Deposit" : "Withdraw");
            success = 1;
            break;
        }
    }

    unlock_file(fd);
    close(fd);
    return success ? 0 : -1;
}

// Transfer Funds
int execute_transfer(const char *from_user, const char *to_user, float amount) {
    if (strcmp(from_user, to_user) == 0) return -1;

    int fd = open("data/customers.dat", O_RDWR);
    if (fd == -1) {
        perror("execute_transfer: open");
        return -1;
    }

    if (lock_file(fd, F_WRLCK) == -1) {
        close(fd);
        return -1;
    }

    struct Customer from, to;
    off_t from_pos = -1, to_pos = -1;

    while (read(fd, &from, sizeof(from)) == sizeof(from)) {
        if (strcmp(from.username, from_user) == 0) {
            from_pos = lseek(fd, -sizeof(from), SEEK_CUR);
            break;
        }
    }

    lseek(fd, 0, SEEK_SET);  // rewind and find receiver
    while (read(fd, &to, sizeof(to)) == sizeof(to)) {
        if (strcmp(to.username, to_user) == 0) {
            to_pos = lseek(fd, -sizeof(to), SEEK_CUR);
            break;
        }
    }

    if (from_pos == -1 || to_pos == -1 || from.balance < amount) {
        unlock_file(fd);
        close(fd);
        return -1;
    }

    from.balance -= amount;
    to.balance += amount;

    lseek(fd, from_pos, SEEK_SET);
    write(fd, &from, sizeof(from));

    lseek(fd, to_pos, SEEK_SET);
    write(fd, &to, sizeof(to));

    unlock_file(fd);
    close(fd);

    record_transaction(from_user, from_user, to_user, amount, "Debit");
    record_transaction(to_user, from_user, to_user, amount, "Credit");

    return 0;
}

// Show transaction history over socket
void show_transaction_history(int client_fd, const char *user_id) {
    char file_path[100];
    snprintf(file_path, sizeof(file_path), "data/%s_txn.dat", user_id);
    int fd = open(file_path, O_RDONLY);

    if (fd == -1) {
        const char *msg = "No transaction history available.\n";
        write(client_fd, msg, strlen(msg));
        return;
    }

    struct Transaction txn;
    char buffer[1024];
    int count = 0;
    while (read(fd, &txn, sizeof(txn)) == sizeof(txn)) {
        int len = snprintf(buffer, sizeof(buffer),
            "TxnID: %s | From: %s | To: %s | Amount: %.2f | Type: %s | Time: %s\n",
            txn.txn_id, txn.sender, txn.receiver, txn.amount, txn.txn_type, txn.timestamp);
        write(client_fd, buffer, len);
        count++;
    }

    if (count == 0)
        write(client_fd, "No transactions found.\n", 24);

    close(fd);
}

// Feedback submission
int save_feedback(const char *user_id, const char *feedback_text) {
    int fd = open(FEEDBACK_FILE, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd == -1) return -1;

    if (lock_file(fd, F_WRLCK) == -1) {
        close(fd);
        return -1;
    }

    char entry[256];
    snprintf(entry, sizeof(entry), "%s: %s\n", user_id, feedback_text);
    write(fd, entry, strlen(entry));

    unlock_file(fd);
    close(fd);
    return 0;
}

// Show last 5 feedbacks
int fetch_recent_feedbacks(int client_fd) {
    int fd = open(FEEDBACK_FILE, O_RDONLY);
    if (fd == -1) {
        write(client_fd, "No feedbacks yet.\n", 18);
        return -1;
    }

    char lines[20][256];
    int count = 0;

    char buf[1];
    int idx = 0;
    char line[256] = {0};

    while (read(fd, buf, 1) == 1) {
        if (buf[0] == '\n' || idx >= 255) {
            line[idx] = '\0';
            if (strlen(line) > 0)
                strncpy(lines[count++], line, sizeof(line));
            idx = 0;
            memset(line, 0, sizeof(line));
            if (count >= 20) count = 20;
        } else {
            line[idx++] = buf[0];
        }
    }

    int start = (count > 5) ? count - 5 : 0;
    for (int i = start; i < count; i++) {
        write(client_fd, lines[i], strlen(lines[i]));
        write(client_fd, "\n", 1);
    }

    close(fd);
    return 0;
}
