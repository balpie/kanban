#include "../include/lavagna_net.h"
#include "../include/lavagna.h"
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
connection_l_e* insert_connection(connection_l_e **headptr, int socket, uint16_t port_id, uint32_t addr)
{
    connection_l_e *nuovo = (connection_l_e*)malloc(sizeof(connection_l_e));
    nuovo->socket = socket;
    nuovo->port_id = port_id;
    nuovo->addr = addr;
    nuovo->next = *headptr;
    nuovo->to_send = NULL;
    *headptr = nuovo;
    return nuovo;
}

// ritorna 0 se rimozione fallita, 1 altrimenti
int remove_connection(connection_l_e **headptr, int sock)
{
    connection_l_e* iter = *headptr;
    connection_l_e* prec = NULL;
    while(iter && iter->socket != sock)
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
        if(close((*headptr)->socket) < 0)
        {
            perror("[remove_connection] errore close");
        }

        *headptr = (*headptr)->next;
        free(iter);
        return 1;
    }
    // rimozione altrove
    if(close(iter->socket) < 0)  
    {
        perror("[remove_connection] errore close");
    }
    prec->next = iter->next;
    free(iter);
    return 1;
}

void send_connection(int sock, connection_l_e* conn)
{
    uint16_t port = htons(conn->port_id); 
    uint32_t addr = htonl(conn->addr);
    send_msg(sock, &port, 2);
    send_msg(sock, &addr, 4);
}

// TODO gestione caso disconnessione di un client 
void send_conn_list(int sock, connection_l_e* escluso, uint8_t quanti)
{
    connection_l_e* p = lista_connessioni.head;
    while(p)
    {
        if(p != escluso && p->to_send)
        {
            send_connection(sock, p);
            quanti--;
        }
        p = p->next;
    }
    connection_l_e dummy;
    dummy.port_id = 5678;
    dummy.addr = 5678;
    // caso disconnessione inaspettata
    // starà ai client comunicare l'esito fallito dell'operazione
    while(quanti > 0)
    {
        quanti--;
        send_connection(sock, &dummy);
    }
}
