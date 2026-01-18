#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <openssl/sha.h>
#include "../include/sha256_utils.h"

// Funzione wrapper per calcolare l'hash SHA256 di un file
void digest_file(const char *filename, uint8_t *hash) {
    SHA256_CTX ctx;       // Contesto OpenSSL
    SHA256_Init(&ctx);    // Inizializzazione

    char buffer[32];      // Buffer di lettura (chunk size)
    int file = open(filename, O_RDONLY);

    // Se il file non si apre, la funzione ritorna (errore gestito dal chiamante con access)
    if(file == -1) return; 

    ssize_t bR;
    // Legge il file a blocchi e aggiorna l'hash parziale
    while((bR = read(file, buffer, 32)) > 0){
        SHA256_Update(&ctx, (uint8_t *)buffer, bR);
    }

    // Genera l'hash finale (32 byte binari)
    SHA256_Final(hash, &ctx);
    close(file);
}