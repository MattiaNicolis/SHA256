#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<fcntl.h>
#include "client.h"
#include "../common/protocol.h"

int main(int argc, char *argv[]) {
    if(argc != 2) {
        printf("Uso: %s <percorso_file>\n", argv[0]);
        exit(1);
    }

    request_t req;
    req.client_pid = getpid();
    strncpy(req.filepath, argv[1], MAX_PATH);

    int fd_req = open(FIFO_REQ, O_WRONLY);
    write(fd_req, &req, sizeof(req));

    int fd_res = open(FIFO_RES, O_RDONLY);
    response_t res;
    read(fd_res, &res, sizeof(res));

    if(res.client_pid == getpid())
        printf("SHA-256: %s\n", res.hash);
}