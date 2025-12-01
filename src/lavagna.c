#include "../include/common.h"
#include "../include/lavagna.h"
#include "../include/lavagna_net.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h> 
#include <pthread.h>

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
                printf(">> comando inesistente\n");
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


void stampa_utenti_connessi(connection_l_e *head)
{
    int count = 0;
    while(head!= NULL)
    {
        printf("%d)Port %u\n", ++count, head->port_id);
        head = head->next;
    }
}

void cleanup(int listener_sock, connection_l_e** list)
{
    close(listener_sock);
    while(*list)
    {
        remove_connection(list, (*list)->socket);
    }
}
