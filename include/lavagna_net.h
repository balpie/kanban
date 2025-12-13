#ifndef LAVAGNA_NET_H
#define LAVAGNA_NET_H
#include "common_net.h"
#include "common.h"

typedef struct connection_list_element connection_l_e;
struct connection_list_element
{
    int socket;
    uint16_t port_id; // porta id client network order
    uint32_t addr; // indirizzo client network order
    connection_l_e *next;
    task_card_t *to_send; // se presente, punta alla carta da mandare
};

struct connection_list
{
    connection_l_e *head;
    pthread_mutex_t m;
};
typedef struct connection_list connection_l;

// inserisce connessione in testa
// funzione da chiamare solo dopo la registrazione
connection_l_e* insert_connection(connection_l_e**, int, uint16_t, uint32_t);
// rimuove connessione
// il socket della connessione 
int remove_connection(connection_l_e **, int);

// Manda via socket la connessione passata
void send_connection(int, connection_l_e*);

// manda via socket tutti i socket con tosend != NULL escludendo "escluso" se presente.
// se il numero non è corretto manda dei socket "falsi", che hanno 5678 come indirizzo e porta
void send_conn_list(int, connection_l_e*, uint8_t);

#endif
