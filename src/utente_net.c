#include "../include/utente_net.h"
#include <stdlib.h>
#include <string.h>

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
