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

int sock_listener; // in modo da poter terminare dai thread
struct server_status status; 
                             
                             
                             
                             

lavagna_t *lavagna = NULL; 
pthread_rwlock_t m_lavagna;

int main(int argc, char *argv[]) // main thread: listener
{
    // uso lazy eval: se ho un argomento solo passo avanti, 
    // in caso contrario se il primo argomento non è -d 
    // scrivo i commmenti in un log file
    if(argc < 2 || strcmp(argv[1], "-d")) 
    {
        // TODO error check
        int logfile = open(LOGFILE_NAME, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        dup2(logfile, STDERR_FILENO); // associo stderr al logfile
        close(logfile);
        fflush(stderr);
    }
    pthread_mutex_init(&status.m, NULL);
    pthread_rwlock_init(&m_lavagna, NULL);
    status.n_connessioni = 0;
    status.winner_arrived = 0;
    struct sockaddr_in listener_addr;
    struct sockaddr_in new_connection;
    sock_listener = init_listener(&listener_addr, LAVAGNA_PORT);
    int new_sock;
    // Inizializzo strutture dati per i thread
    pthread_t prompt_thread;
    pthread_t server_processes; 
    pthread_mutex_init(&lista_connessioni.m, 0);
    pthread_cond_init(&status.cv, NULL);
    // creo il thread per interazione via terminale
    pthread_create(&prompt_thread, NULL, prompt_cycle, NULL); 
    while(1)
    {
        unsigned int len_new = sizeof(new_connection);
        if(status.n_connessioni < MAX_SERVER_PROCS)
        {
            new_sock = accept(sock_listener, (struct sockaddr*)&new_connection, &len_new);
            // alloco nello heap altrimenti in caso di connessioni molto vicine tra loro
            // se fosse nello stack del main rischierei che si sovrascrivessero dandomi
            // strutture dati inconsistenti
            struct client_info *ci = (struct client_info*)malloc(sizeof(struct client_info));
            ci->addr = new_connection.sin_addr.s_addr;
            ci->socket = new_sock;
            pthread_create(&server_processes, NULL, serv_client, ci);
            pthread_detach(server_processes);
        }
        else 
        {// se ho tutti i thread occupati aspetto che uno si liberi.
            sleep(1);
        }
        fprintf(stderr, "[dbg] listener: numero connessioni: %d\n", status.n_connessioni);
    }
    return 0;
}
