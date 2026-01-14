#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "../include/common.h"

int main(int argc, char *argv[]) {
    if(argc < 2) {
        printf("Utilizzo: %s <percorso_file>\n", argv[0]);
        return 1;
    }

    char *target_file = argv[1];
    struct stat st;

    // Controllo esistenza file locale
    if(stat(target_file, &st) != 0) {
        perror("Errore nell'apertura del file target");
        return 1;
    }

    // Creazione FIFO privata
    char my_fifo[MAX_PATH];
    memset(my_fifo, 0, MAX_PATH);
    snprintf(my_fifo, MAX_PATH, "/tmp/client_%d_fifo", getpid());

    unlink(my_fifo);
    if (mkfifo(my_fifo, 0666) == -1) {
        perror("Errore creazione FIFO privata");
        return 1;
    }

    // Preparazione richiesta
    request_t req;
    memset(&req, 0, sizeof(request_t)); // Pulizia importante!
    req.type = REQ_COMPUTE;
    strncpy(req.file_path, target_file, MAX_PATH - 1);
    strncpy(req.client_fifo, my_fifo, MAX_PATH - 1);
    req.file_size = st.st_size;

    // Invio al server
    int server_fd = open(SERVER_FIFO, O_WRONLY);
    if(server_fd == -1) {
        printf("Errore: il server non sembra essere attivo.\n");
        unlink(my_fifo);
        return 1;
    }

    printf("Richiesta inviata: %s (%ld byte)\n", target_file, req.file_size);
    write(server_fd, &req, sizeof(request_t));
    close(server_fd);

    // Attesa risposta
    printf("In attesa dell'hash...\n");
    int my_fd = open(my_fifo, O_RDONLY);
    if (my_fd == -1) {
        perror("Errore apertura FIFO privata in lettura");
        unlink(my_fifo);
        return 1;
    }

    response_t res;
    ssize_t n = read(my_fd, &res, sizeof(response_t));
    if (n > 0) {
        if (res.from_cache) {
            printf("[Cache Hit] ");
        } else {
            printf("[Calcolato] ");
        }
        printf("SHA256: %s\n", res.hash);
    } else {
        printf("Errore o nessuna risposta ricevuta.\n");
    }

    close(my_fd);
    unlink(my_fifo);

    return 0;
}