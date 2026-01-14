#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <openssl/sha.h>
#include "../include/sha256_utils.h"

void digest_file(const char *filename, uint8_t *hash) {
    SHA256_CTX ctx;
    SHA256_Init(&ctx);

    char buffer[32];
    int file = open(filename, O_RDONLY);

    // Se il file non si apre, ritorniamo semplicemente (gestito dal server con access)
    if(file == -1) return; 

    ssize_t bR;
    while((bR = read(file, buffer, 32)) > 0){
        SHA256_Update(&ctx, (uint8_t *)buffer, bR);
    }

    SHA256_Final(hash, &ctx);
    close(file);
}