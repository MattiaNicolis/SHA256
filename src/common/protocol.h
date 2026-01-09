#include<sys/types.h>
#ifndef PROTOCOL_H
#define PROTOCOL_H

#define FIFO_REQ "fifo/requests.fifo"
#define FIFO_RES "fifo/responses.fifo"

#define MAX_PATH 256
#define HASH_LEN 65

typedef struct {
    pid_t client_pid;
    char filepath[MAX_PATH];
} request_t;

typedef struct {
    pid_t client_pid;
    char hash[HASH_LEN];
} response_t;

#endif