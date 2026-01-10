#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <pthread.h>
#include<openssl/sha.h>

#include "server.h"
#include "../common/protocol.h"

void digstfile(const char *filename, unsigned char *hash) {
    SHA256_CTX ctx;
    SHA256_Init(&ctx);

    char buffer[32];
    int fd = open(filename, O_RDONLY);
    if(fd < 0) {
        perror("open");
        return;
    }

    ssize_t n;
    while((n = read(fd, buffer, sizeof(buffer))) > 0) {
        SHA256_Update(&ctx, buffer, n);
    }

    SHA256_Final(hash, &ctx);
    close(fd);
}

void hash_to_string(unsigned char *hash, char *output) {
    for(int i=0; i< 32; i++) {
        sprintf(output + (i*2), "%02x", hash[i]);
    }

    output[64] = '\0';
}

void* handle_request(void* arg) {
    request_t* req = (request_t*)arg;

    printf("[THREAD] PID %d, file: %s\n",
           req->client_pid, req->filepath);

    response_t res;
    res.client_pid = req->client_pid;
    
    unsigned char hash[32];
    digstfile(req->filepath, hash);
    hash_to_string(hash, res.hash);

    int fd_res = open(FIFO_RES, O_WRONLY);
    write(fd_res, &res, sizeof(res));
    close(fd_res);

    free(req);
    return NULL;
}

int main() {
    mkfifo(FIFO_REQ, 0666);
    mkfifo(FIFO_RES, 0666);

    printf("Server avviato...\n");

    int fd_req = open(FIFO_REQ, O_RDONLY);

    while (1) {
        request_t* req = malloc(sizeof(request_t));

        ssize_t n = read(fd_req, req, sizeof(request_t));

        if(n==0) {
            free(req);
            close(fd_req);
            fd_req = open(FIFO_REQ, O_RDONLY);
            continue;
        }

        if(n < 0) {
            perror("read");
            free(req);
            continue;
        }

        pthread_t tid;
        pthread_create(&tid, NULL, handle_request, req);
        pthread_detach(tid);
    }
}