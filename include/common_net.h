#ifndef COMMON_NET_H
#define COMMON_NET_H

#define RTR_CONN_CLOSE 0
// ho scelto newline perchè è un carattere che l'utente
// non può inserire in descrizione
#define PING_PONG_MSG (char)0

#define LAVAGNA_PORT 5678
#define LAVAGNA_ADDR "127.0.0.1"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "common.h"

// implementazione di htonll
uint64_t htonll(uint64_t num);
uint64_t ntohll(uint64_t num);

// funzione che inizializza socket listener
// ritorna il socket listener o -1 al fallimento
int init_listener(struct sockaddr_in*);

// manda size byte di msg via sock
// ritorna 1 se eseguita con successo 0 altrimenti
int send_msg(int, void*, size_t);

// riceve messaggio dal socket (argomento 1) un messaggio di dimensione (argomento 2)
int get_msg(int, void*, size_t);

// send card al socket passato per indirizzo
int send_card(int, task_card_t *);

// socket da cui ricevere la card, dimensione card
task_card_t* recive_card(int, size_t);

#endif
