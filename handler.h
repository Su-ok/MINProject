#ifndef HANDLER_H
#define HANDLER_H

#include <pthread.h>

// Handle a client connection (run in its own thread)
void *handle_client(void *socket_desc);

// Individual role handlers
void handle_customer(int sock);
void handle_employee(int sock);
void handle_manager(int sock);
void handle_admin(int sock);

#endif
