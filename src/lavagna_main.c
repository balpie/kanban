#include "../include/common.h"
#include "../include/lavagna.h"
#include "../include/lavagna_net.h"
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

struct client_info{
    int socket;
    uint32_t addr; // network order
};

connection_l lista_connessioni;
int n_connessioni = 0;
int sock_listener; // in modo da poter terminare dai thread

void* prompt_cycle(void *)
{
    char cmd;
    do{
        cmd = prompt_line("lavagna");
        switch(cmd)
        {
            case CMD_NOP:
                break;
            case CMD_INVALID:
                printf("Comando inesistente\n");
                break;
            case CMD_STAMPA_UTENTI_CONNESSI:
                pthread_mutex_lock(&lista_connessioni.m);
                stampa_utenti_connessi(lista_connessioni.head);
                pthread_mutex_unlock(&lista_connessioni.m);
                break;
        }
    }while(cmd != CMD_QUIT);
    printf(">> ripulisco socket e heap...\n");  
    cleanup(sock_listener, &lista_connessioni.head);
    exit(0);
}
  
// argomento passato: connection_l relativo al client da servire
void* serv_client(void* cl_info) 
{
    struct client_info *cl_in = (struct client_info*)cl_info;
    int socket = cl_in->socket;
    uint32_t addr = cl_in->addr;
    free(cl_in);
    uint16_t port; // port_id
    // il primo messaggio che ogni client deve mandare è la propria porta
    // identificativa, che gli permette di parlare con gli altri
    get_msg(socket, (void*)&port, 2); 
    pthread_mutex_lock(&lista_connessioni.m);
    insert_connection(&(lista_connessioni.head), socket, ntohs(port), addr); 
    pthread_mutex_unlock(&lista_connessioni.m);
    n_connessioni++;
    return NULL;
}

int main() // main thread: listener
{
    struct sockaddr_in listener_addr;
    struct sockaddr_in new_connection;
    sock_listener = init_listener(&listener_addr);
    int new_sock;
    pthread_t prompt_thread;
    pthread_t server_processes[MAX_SERVER_PROCS];
    pthread_mutex_init(&lista_connessioni.m, 0);
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
