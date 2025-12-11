#include "../include/utente_net.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// implementazione parte p2p utente
peer_list *recive_peer(int sock)
{
    unsigned char buf[6];
    get_msg(sock, buf, 6);
    peer_list* new = (peer_list*)malloc(sizeof(peer_list));
    memcpy(&new->port, &buf[0], 2);
    memcpy(&new->addr, &buf[2], 4);

    new->addr = ntohl(new->addr);
    new->port = ntohs(new->port);

    new->next = NULL;
    return new;
}

// Inserisce il peer nella lista, in modo da rendere la lista ordinata rispetto al
// numero di porta in modo crescente
void insert_peer(peer_list** pl, peer_list* elem)
{
    
    fprintf(stderr, "[dbg] partita insert_peer, *pl = %p\n", *pl);
    while(*pl && (*pl)->port < elem->port)
    {
        fprintf(stderr, "[dbg] insert_peer, scorro dopo peer con porta: %u\n", (*pl)->port);
        pl = &((*pl)->next);
    }
    peer_list* tmp = *pl;
    *pl = elem;
    elem->next = tmp;
}

// Libera la memoria della peer_list
void deallocate_list(peer_list** pl)
{
    while(*pl)
    {
        peer_list* tmp = *pl;
        pl = &((*pl)->next);
        free(tmp);
    }
}

void print_peers(peer_list* list)
{
    int cont = 0;
    while(list)
    {
        cont++;
        fprintf(stderr, "[dbg]: -%d- addr %u\tport %u\n", cont, list->addr, list->port);
        list = list->next;
    }
}
