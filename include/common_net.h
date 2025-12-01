#ifndef COMMON_NET_H
#define COMMON_NET_H

#define LAVAGNA_PORT 5678
#define LAVAGNA_ADDR "127.0.0.1"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// funzione che inizializza socket listener
// ritorna il socket listener o -1 al fallimento
int init_listener(struct sockaddr_in*);

// manda size byte di msg via sock
// ritorna 1 se eseguita con successo 0 altrimenti
int send_msg(int, void*, size_t);

// riceve messaggio dal socket (argomento 1) un messaggio di dimensione (argomento 2)
void* get_msg(int, void*, size_t);

#endif
