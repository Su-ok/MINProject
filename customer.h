// customer.h
#ifndef CUSTOMER_H
#define CUSTOMER_H

#include <unistd.h>
#include "common.h"

// Structure for transaction records
struct Transaction {
    char txn_id[20];
    char sender[MAX_NAME];
    char receiver[MAX_NAME];
    float amount;
    char txn_type[20];
    char timestamp[25];
};

// ---- Customer Operations ----

// Check balance for a given customer
float fetch_account_balance(const char *user_id);

// Deposit or withdraw amount (+ve for deposit, -ve for withdrawal)
int modify_balance(const char *user_id, float delta);

// Transfer funds between two customers
int execute_transfer(const char *from_user, const char *to_user, float amount);

// Record a transaction in a .dat log
void record_transaction(const char *user, const char *sender, const char *receiver, float amt, const char *type);

// Fetch and send transaction history over socket
void show_transaction_history(int client_fd, const char *user_id);

// Submit feedback
int save_feedback(const char *user_id, const char *feedback_text);

// Retrieve the last few feedbacks
int fetch_recent_feedbacks(int client_fd);

#endif
