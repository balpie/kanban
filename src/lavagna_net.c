#include "../include/lavagna_net.h"
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
// ritorna il socket listener o -1 al fallimento
int init_listener(struct sockaddr_in* server_addr)
{
    int listener = socket(AF_INET, SOCK_STREAM, 0);
    if(listener < 0)
    {
        return -1;
    }
    memset(server_addr, 0, sizeof(struct sockaddr_in));
    server_addr->sin_addr.s_addr = INADDR_ANY;
    server_addr->sin_family = AF_INET;
    server_addr->sin_port = htons(LAVAGNA_PORT);
    if(bind(listener, (struct sockaddr*)server_addr, sizeof(*server_addr)) < 0)
    {
        perror("[init_listener] errore bind");
        exit(-1);
    }
    if(listen(listener, MAX_QUEUE) < 0)
    {
        perror("[init_listener] errore listen");
        exit(-1);
    }
    return listener;
}

// inserimento in testa
void insert_connection(connection_l_e **headptr, struct sockaddr_in *addr, int socket)
{
    connection_l_e *nuovo = (connection_l_e*)malloc(sizeof(connection_l_e));
    nuovo->socket = socket;
    nuovo->indirizzo.sin_family = addr->sin_family;
    nuovo->indirizzo.sin_port = addr->sin_port;
    nuovo->indirizzo.sin_addr.s_addr = addr->sin_addr.s_addr;
    nuovo->next = *headptr;
    *headptr = nuovo;
}

// ritorna 0 se rimozione fallita, 1 altrimenti
int remove_connection(connection_l_e **headptr, struct sockaddr_in *addr)
{
    if(!*headptr) // nulla da rimuovere
    { 
        return 0;
    }
    connection_l_e* iter = *headptr;
    connection_l_e* prec = NULL;
    while(iter 
            && addr->sin_port != iter->indirizzo.sin_port 
            && addr->sin_addr.s_addr != iter->indirizzo.sin_addr.s_addr)
    {
        prec = iter;
        iter = iter->next;
    }
    if(!iter) // arrivato in fondo senza trovare addr
    { 
        return 0;
    }
    if(!prec) // rimozione in testa
    {
        if (close((*headptr)->socket) < 0)
        {
            perror("[remove_connection] errore close");
        }
        *headptr = (*headptr)->next;
        free(iter);
        return 1;
    }
    // rimozione altrove
    if (close(iter->socket) < 0)
    {
        perror("[remove_connection] errore close");
    }
    prec->next = iter->next;
    free(iter);
    return 1;
}

