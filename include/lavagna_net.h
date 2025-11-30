#ifndef LAVAGNA_NET_H
#define LAVAGNA_NET_H
#include "common_net.h"

#define MAX_QUEUE 10

typedef struct connection_list_element connection_l_e;
struct connection_list_element
{
    int socket;
    uint16_t port_id; // porta id client network order
    uint32_t addr; // indirizzo client network order
    connection_l_e *next;
};

struct connection_list
{
    connection_l_e *head;
    pthread_mutex_t m;
};

typedef struct connection_list connection_l;

// funzione che inizializza socket listener
// ritorna il socket listener o -1 al fallimento
int init_listener(struct sockaddr_in*);
// inserisce connessione in testa
// funzione da chiamare solo dopo la registrazione
void insert_connection(connection_l_e**, int, uint16_t, uint32_t);
// rimuove connessione
// il socket della connessione 
int remove_connection(connection_l_e **, int);

#endif
