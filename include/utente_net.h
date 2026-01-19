#ifndef UTENTE_NET_H
#define UTENTE_NET_H

#include "../include/common_net.h"

#define IP_LAVAGNA "127.0.0.1"
#define PORTA_LAVAGNA 5678  

// Protocollo P2P kanban
// I peer si ascolteranno dalla peer list in ordine di port. Il primo peer a 
// comunicare deve essere quello con la port più bassa tra tutti, e così via.
struct peer_list_element{
    int sock;
    uint32_t addr;
    uint16_t port;
    struct peer_list_element *next;
};
typedef struct peer_list_element peer_list;

// riceve singolo peer dal server, via socket passato come argomento
peer_list *recive_peer(int);

// Inserisce il peer nella lista, in modo da rendere la lista ordinata 
// rispetto al numero di porta in modo crescente
void insert_peer(peer_list**, peer_list*);

// Libera la memoria della peer_list
void deallocate_list(peer_list**);

// stampa i peer nella lista (debug)
void print_peers(peer_list*);

// inizializza i socket: 
int init_sockets(peer_list*);

// la seguente funzione, in base al protocollo kanban, ascolta sulla propria 
// porta, o comunica alla porta di tutti il proprio costo. l'argomento passato 
// è il peer con numero di porta più basso all'interno della propria lista, 
// che non ha ancora avuto il turno. 
// arg1: socket listener
// arg2: primo elemento lista peer
// arg3: elemento corrente lista peer
// arg4: numero di porta dell'utente
// arg5: dopo la funzione punterà a intero senza segno (già allocato) 
//       che è il peer con costo minimo fino ad adesso
unsigned kanban_p2p_iteration(int, peer_list*, peer_list*, 
        uint16_t, uint16_t*);

// timeout socket utente (sia recv che send)
extern struct timeval timeout_p2p;

#endif
