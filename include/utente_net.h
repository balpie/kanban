#ifndef UTENTE_NET_H
#define UTENTE_NET_H

#include "../include/common_net.h"

#define IP_LAVAGNA "127.0.0.1"
#define PORTA_LAVAGNA 5678  

// I peer si ascolteranno dalla peer list in ordine di port.
// Il primo peer a comunicare deve essere quello con la port più bassa tra tutti, e così via.
// TODO: mancata trasmissione da parte di un peer
struct peer_list_element{
    uint32_t addr;
    uint16_t port;
    struct peer_list_element *next;
};
typedef struct peer_list_element peer_list;

// riceve singolo peer dal server, via socket passato come argomento
peer_list *recive_peer(int);

// Inserisce il peer nella lista, in modo da rendere la lista ordinata rispetto al
// numero di porta in modo crescente
void insert_peer(peer_list**, peer_list*);

// Libera la memoria della peer_list
void deallocate_list(peer_list**);

// stampa i peer nella lista (debug)
void print_peers(peer_list*);

#endif
