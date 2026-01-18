#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include "../include/common.h"
#include "../include/sha256_utils.h"

#define MAX_THREADS 4 // Dimensione del Thread Pool

// --- STRUTTURE DATI ---

// Nodo per la coda delle richieste (Lista Linkata)
typedef struct request_node {
    request_t data;
    struct request_node *next;
} request_node_t;

// Nodo per la cache dei risultati (Lista Linkata)
typedef struct cache_node {
    char path[MAX_PATH];
    char hash[HASH_SIZE];
    struct cache_node *next;
} cache_node_t;

// --- VARIABILI GLOBALI CONDIVISE ---
request_node_t *request_queue = NULL; // Coda delle richieste da elaborare
cache_node_t *hash_cache = NULL;      // Cache dei file già calcolati

// --- SINCRONIZZAZIONE ---
// Mutex unico per proteggere sia la coda che la cache (Global Lock)
pthread_mutex_t server_mutex = PTHREAD_MUTEX_INITIALIZER;
// Condition Variable per svegliare i worker quando c'è lavoro
pthread_cond_t request_cond = PTHREAD_COND_INITIALIZER;

/**
 * Funzione: enqueue_request
 * Scopo: Inserisce una richiesta nella coda.
 * Logica: Implementa lo scheduling SJF (Shortest Job First).
 * La lista viene mantenuta ordinata in base a req.file_size.
 */
void enqueue_request(request_t req) {
    request_node_t *new_node = malloc(sizeof(request_node_t));
    new_node->data = req;
    new_node->next = NULL;

    // INIZIO SEZIONE CRITICA
    pthread_mutex_lock(&server_mutex);

    // Se la coda è vuota o il nuovo file è più piccolo del primo -> inserisci in testa
    if(!request_queue || req.file_size < request_queue->data.file_size) {
        new_node->next = request_queue;
        request_queue = new_node;
    } else {
        // Altrimenti scorri la lista per trovare la posizione giusta (ordinamento crescente)
        request_node_t *curr = request_queue;
        while(curr->next && curr->next->data.file_size <= req.file_size) {
            curr = curr->next;
        }
        // Inserimento del nodo
        new_node->next = curr->next;
        curr->next = new_node;
    }

    // Segnala ai worker che c'è una nuova richiesta
    pthread_cond_signal(&request_cond);
    
    // FINE SEZIONE CRITICA
    pthread_mutex_unlock(&server_mutex);
}

/**
 * Funzione: worker_thread
 * Scopo: Ciclo di vita dei thread operai.
 * Preleva richiesta -> Controlla Cache -> Calcola -> Risponde.
 */
void *worker_thread(void *arg) {
    while(1) {
        // 1. PRELIEVO RICHIESTA (Consumer)
        pthread_mutex_lock(&server_mutex);

        // Se la coda è vuota, dormi e aspetta un segnale (evita Busy Waiting)
        while(!request_queue) {
            pthread_cond_wait(&request_cond, &server_mutex);
        }

        // Estrai il primo elemento (testa della coda)
        request_node_t *node = request_queue;
        request_queue = request_queue->next;
        pthread_mutex_unlock(&server_mutex);

        // Copia i dati e libera la memoria del nodo
        request_t req = node->data;
        free(node);

        response_t res;
        memset(&res, 0, sizeof(response_t)); // Pulizia memoria fondamentale
        res.from_cache = 0;
        res.status = 0;

        // 2. CONTROLLO CACHE (Lettura)
        pthread_mutex_lock(&server_mutex);
        cache_node_t *c = hash_cache;
        while(c) {
            if(strcmp(c->path, req.file_path) == 0) {
                // Trovato! Copia l'hash e salta il calcolo
                strcpy(res.hash, c->hash);
                res.from_cache = 1;
                break;
            }
            c = c->next;
        }
        pthread_mutex_unlock(&server_mutex);

        // 3. CALCOLO DELL'HASH (Se non era in cache)
        if(!res.from_cache) {
            // Controllo sicurezza: il file esiste ancora?
            if(access(req.file_path, F_OK) != 0) {
                printf("Errore: File %s non trovato.\n", req.file_path);
                strcpy(res.hash, "ERRORE_FILE_NOT_FOUND");
            } else {
                uint8_t raw_hash[32];
                digest_file(req.file_path, raw_hash); // Funzione lenta CPU-bound

                // Conversione da binario a stringa esadecimale
                for(int i=0; i<32; i++)
                    sprintf(res.hash + (i * 2), "%02x", raw_hash[i]);
                res.hash[64] = '\0'; // Terminatore stringa

                // 4. AGGIORNAMENTO CACHE (Scrittura)
                pthread_mutex_lock(&server_mutex);
                cache_node_t *new_cache = malloc(sizeof(cache_node_t));
                strncpy(new_cache->path, req.file_path, MAX_PATH);
                strcpy(new_cache->hash, res.hash);
                new_cache->next = hash_cache; // Inserimento in testa (più semplice)
                hash_cache = new_cache;
                pthread_mutex_unlock(&server_mutex);
            }
        }

        // 5. INVIO RISPOSTA AL CLIENT
        // Apre la FIFO privata del client in scrittura
        int cfd = open(req.client_fifo, O_WRONLY);
        if(cfd != -1) {
            write(cfd, &res, sizeof(response_t));
            close(cfd); // Chiude subito dopo l'invio
            printf("Risposta inviata a: %s (Hash: %.10s...)\n", req.client_fifo, res.hash);
        } else {
            perror("Errore apertura FIFO client");
        }
    }
    return NULL;
}

// Gestore segnale CTRL+C per pulizia ordinata
void handle_shutdown(int sig) {
    printf("\nSpegnimento del server in corso...\n");
    unlink(SERVER_FIFO); // Rimuove il file della FIFO pubblica
    exit(0);
}

int main() {
    // Rimuove eventuali FIFO residue da esecuzioni precedenti
    unlink(SERVER_FIFO);

    // Crea la FIFO pubblica (Named Pipe)
    if(mkfifo(SERVER_FIFO, 0666) == -1) {
        perror("Errore creazione FIFO server");
        exit(1);
    }

    // Creazione del Thread Pool
    pthread_t pool[MAX_THREADS];
    for(int i=0; i<MAX_THREADS; i++)
        pthread_create(&pool[i], NULL, worker_thread, NULL);
    
    printf("Server avviato. In ascolto su %s...\n", SERVER_FIFO);

    // Apertura FIFO. Nota: O_RDWR è usato su macOS/Linux per impedire
    // che la read ritorni EOF ogni volta che un client si scollega.
    int sfd = open(SERVER_FIFO, O_RDWR);
    if(sfd == -1) {
        perror("Errore apertura FIFO server");
        exit(1);
    }

    // Registra il gestore per SIGINT (CTRL+C)
    signal(SIGINT, handle_shutdown);

    // Ciclo principale del Dispatcher
    while(1) {
        request_t req;
        // Lettura bloccante dalla FIFO pubblica
        ssize_t n = read(sfd, &req, sizeof(request_t));

        if(n > 0) {
            // Smistamento richieste
            if(req.type == REQ_QUERY_CACHE) {
                // Gestione Query Cache (Codice per cache_query.c)
                // ... (omesso per brevità, identico al precedente) ...
                // Nota: Qui stampiamo il contenuto della cache scorrendo la lista
            } 
            else if(req.type == REQ_COMPUTE) {
                // Gestione Calcolo standard
                printf("Ricevuta richiesta calcolo per: %s\n", req.file_path);
                // Il Main Thread NON calcola, ma delega mettendo in coda (Producer)
                enqueue_request(req);
            }
        }
    }
    return 0;
}