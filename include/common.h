#ifndef COMMON_H
#define COMMON_H

#include<stdint.h>

#define SERVER_FIFO "/tmp/sha256_server_fifo"
#define MAX_PATH 256
#define HASH_SIZE 65 // 64 caratteri hex + terminatore NULL

typedef enum {
    REQ_COMPUTE,
    REQ_QUERY_CACHE,
    REQ_EXIT
} req_type_t;

typedef struct {
    req_type_t type;
    char file_path[MAX_PATH];
    char client_fifo[MAX_PATH];
    long file_size;
} request_t;

typedef struct {
    char hash[HASH_SIZE];
    int status;
    int from_cache;
} response_t;

#endif