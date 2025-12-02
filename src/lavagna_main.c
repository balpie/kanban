#include "../include/common.h"
#include "../include/lavagna.h"
#include "../include/lavagna_net.h"
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

connection_l lista_connessioni;
int n_connessioni = 0;
int sock_listener; // in modo da poter terminare dai thread

lavagna_t *lavagna = NULL; // TODO aggiungi pthread_rw_lock

int main() // main thread: listener
{
    struct sockaddr_in listener_addr;
    struct sockaddr_in new_connection;
    sock_listener = init_listener(&listener_addr);
    int new_sock;
    // Inizializzo strutture dati per i thread
    pthread_t prompt_thread;
    pthread_t server_processes[MAX_SERVER_PROCS];
    pthread_mutex_init(&lista_connessioni.m, 0);
    // creo il thread per interazione via terminale
    pthread_create(&prompt_thread, NULL, prompt_cycle, NULL);
    while(1)
    {
        unsigned int len_new = sizeof(new_connection);
        if(n_connessioni < MAX_SERVER_PROCS)
        {
            new_sock = accept(sock_listener, (struct sockaddr*)&new_connection, &len_new);
            // alloco nello heap altrimenti in caso di connessioni molto vicine tra loro
            // se fosse nello stack del main rischierei che si sovrascrivessero dandomi
            // strutture dati inconsistenti
            struct client_info *ci = (struct client_info*)malloc(sizeof(struct client_info));
            ci->addr = new_connection.sin_addr.s_addr;
            ci->socket = new_sock;
            pthread_create(&server_processes[n_connessioni++], NULL, serv_client, ci);
        }
        else 
        {// se ho tutti i thread occupati aspetto che uno si liberi.
            sleep(1);
        }
    }
    return 0;
}
