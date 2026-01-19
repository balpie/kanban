#include "../include/lavagna_net.h"
#include "../include/lavagna.h"
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

// inserimento in testa
connection_l_e* insert_connection(connection_l_e **headptr, 
        int socket, uint16_t port_id, uint32_t addr)
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

// Manda la connessione al client.
void send_connection(int sock, connection_l_e* conn)
{
    uint16_t port = htons(conn->port_id); 
    uint32_t addr = htonl(conn->addr);
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

// Manda tutte le connessioni al client, esclusa quella puntata da escluso
void send_conn_list(int sock, connection_l_e* escluso, uint8_t quanti)
{
    unsigned mandati = 0;
    connection_l_e* p = lista_connessioni.head;
    for(; p != NULL && quanti > 0; p = p->next)
    {
        if(p != escluso && p->to_send)
        {
            send_connection(sock, p);
            mandati++;
            quanti--;
        }
    }
    // se li ho mandati tutti ho finito
    if(!quanti) 
    {
        return;
    }
    ERR("send_connection_list: disconnessione improvvisa, "
            "mancano %u da mandare", quanti);
    // caso disconnessione inaspettata
    // starà ai client comunicare l'esito fallito dell'operazione.
    // Comunque mando delle connessioni fantoccio per avere simmetria tra i byte mandati 
    // dal server e i byte ricevuti dal client
    connection_l_e dummy;
    dummy.port_id = NO_USR;
    dummy.addr = 0;
    while(quanti > 0)
    {
        LOG("send_conn_list: caso disconnessione inaspettata\n");
        quanti--;
        send_connection(sock, &dummy);
    }
}
