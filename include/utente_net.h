#ifndef UTENTE_NET_H
#define UTENTE_NET_H

#include "../include/common_net.h"

#define IP_LAVAGNA "127.0.0.1"
#define PORTA_LAVAGNA 5678  

struct peer_list_element{
    uint32_t addr;
    uint16_t port;
    struct peer_list_element *next;
};
typedef struct peer_list_element peer_list;

// riceve singolo peer dal server, via socket passato come argomento

peer_list *recive_peer(int);

#endif
