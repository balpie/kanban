#include "../include/lavagna_net.h"
#include "../include/lavagna.h"
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
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
    LOG("send_connection, port: %u\taddr: 0x%X\n", ntohs(port), ntohl(addr));
    unsigned msglen = send_msg(sock, &port, 2);
    if(!msglen)
    {
        ERR("send_connection: port fallita");
    }
    msglen = send_msg(sock, &addr, 4);
    if(!msglen)
    {
        ERR("send_connection: addr fallita");
    }
}

void send_conn_list(int sock, connection_l_e* escluso, uint8_t quanti)
{
    // DEBUG
    connection_l_e *head = lista_connessioni.head;
    int count = 0;
    DBG("- senc_conn_list(%u) ci sono %d utenti connessi in totale: \n",escluso->port_id, status.n_connessioni);
    while(head!= NULL)
    {
        DBG("%d port %u\n", ++count, head->port_id);
        head = head->next;
    }
    // DEBUG

    unsigned mandati = 0;
    connection_l_e* p = lista_connessioni.head;
    for(; p != NULL && quanti > 0; p = p->next)
    {
        DBG("send_conn_list(%u), to_send: %p, port: %u\n", 
                escluso->port_id, p->to_send, p->port_id);
        if(p != escluso && p->to_send)
        {
            send_connection(sock, p);
            mandati++;
            quanti--;
        }
    }
    if(!quanti) // se li ho mandati tutti ho finito
    {
        DBG("send_conn_list(%u) mandati %u\n", escluso->port_id, mandati);
        return;
    }
    ERR("send_connection_list: disconnessione improvvisa, mancano %u da mandare", quanti);
    connection_l_e dummy;
    dummy.port_id = NO_USR;
    dummy.addr = 0;
    // caso disconnessione inaspettata
    // starà ai client comunicare l'esito fallito dell'operazione
    while(quanti > 0)
    {
        LOG("send_conn_list: caso disconnessione inaspettata\n");
        quanti--;
        send_connection(sock, &dummy);
    }
}
