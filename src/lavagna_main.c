#include "../include/common.h"
#include "../include/lavagna.h"
#include "../include/lavagna_net.h"
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

connection_l lista_connessioni;
int sock_listener; 
struct server_status status; 

// Lavagna attività e relativo rwlock
lavagna_t *lavagna = NULL; 
pthread_rwlock_t m_lavagna;

// Timeout ricezione. E' il tempo che la lavagna aspetta 
// che l'utente connesso mandi un messaggio.
// Se non arriva nulla al termine del messaggio si assume 
// che l'utente non abbia nessun istruzione
// da inoltrare alla lavagna
struct timeval timeout_recv = {
    .tv_sec = 0,
    .tv_usec = 250000
};      

// La funzione main si occupa di inizializzare le strutture dati e 
// le strutture per la thread safety, e del ciclo di listen e creazione thread
// per ogni nuova connessione
int main(int argc, char *argv[]) 
{
    if(argc < 2 || strcmp(argv[1], "-d")) 
    {
        int logfile = open(LOGFILE_NAME, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if(logfile < 0)
        {
            printf(">! errore open logfile%s\n", LOGFILE_NAME);
        }
        if(dup2(logfile, STDERR_FILENO) < 0) // associo stderr al logfile
        {
            printf(">! errore ridirezione stderr\n");
        }
        if(close(logfile) < 0)
        {
            printf(">! errore close\n");
        }
        fflush(stderr);
    }
    // Inizializzazione strutture dati
    LOG("Se esiste, aggiungo le card del file %s\n", INITIAL_CARDS);
    if(init_lavagna())
    {
        LOG("Lavagna inizializzata correttamente\n");
    }
    else
    {
        LOG("Lavagna vuota");
    }
    pthread_mutex_init(&status.m, NULL);
    pthread_rwlock_init(&m_lavagna, NULL);
    status.n_connessioni = 0;
    status.winner_arrived = 0;
    status.status = INSTR_NOP;
    struct sockaddr_in listener_addr;
    struct sockaddr_in new_connection;
    sock_listener = init_listener(&listener_addr, LAVAGNA_PORT);
    int new_sock;
    pthread_t prompt_thread;
    pthread_t server_processes; 
    pthread_mutex_init(&lista_connessioni.m, 0);
    pthread_cond_init(&status.cv, NULL);
    pthread_cond_init(&status.fa, NULL);

    // creo il thread per interazione via terminale.
    // Questo è utile per verificare lo stato della lavagna senza
    // bisogno di stampe
    pthread_create(&prompt_thread, NULL, prompt_cycle, NULL); 
    while(1)
    {
        unsigned int len_new = sizeof(new_connection);
        if(status.n_connessioni < MAX_SERVER_PROCS)
        {
            new_sock = accept(sock_listener, 
                    (struct sockaddr*)&new_connection, &len_new);
            LOG("do al socket timeout 1s in send\n");
            if (setsockopt (new_sock, SOL_SOCKET, SO_RCVTIMEO, 
                        &timeout_recv, sizeof(timeout_recv)) < 0)
                ERR("errore setsockopt\n");
            // alloco nello heap altrimenti in caso di connessioni molto 
            // vicine tra loro se fosse nello stack del main rischierei che 
            // si sovrascrivessero dandomi strutture dati inconsistenti
            struct client_info *ci = 
                (struct client_info*)malloc(sizeof(struct client_info));
            ci->addr = new_connection.sin_addr.s_addr;
            ci->socket = new_sock;
            pthread_create(&server_processes, NULL, serv_client, ci);
            pthread_detach(server_processes);
        }
        else 
        { 
            // aspetto che qualcuno si disconnetta prima di accettare una 
            // nuova conessione
            sleep(1);
        }
    }
    return 0;
}
