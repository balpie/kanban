#ifndef COMMON_NET_H
#define COMMON_NET_H

#define RTR_CONN_CLOSE 0

// Definizioni macro che iniziano con INSTR relative a "comandi"  inviati dal
// client al server o viceversa 
// Il server usa alcuni di questi come stati 

// no card disponibili in todo, quindi l'utente non ha nulla da fa
#define INSTR_NOP 0             
// Lista da mandare vuota
#define INSTR_EMPTY 1           
// port o task id già presa
#define INSTR_TAKEN 2           
// Registrazione rifiutata per asta in corso
#define INSTR_WAIT 11           
// Card o utente ricevuto validi e registrati 
#define INSTR_ACK 3             
// Presenti almeno 2 utenti, e almeno 1 card in colonna TO-DO
#define INSTR_AVAL_CARD 4       
                                
// Parte il meccanismo di assegnazione della card
// server comunica ai client che sono pronti per far partire
// l'interazione di tipo p2p, in quanto hanno tutti ricevuto e 
// fatto acknowledge della lista dei peer e della card disponibile 
#define INSTR_CLIENTS_READY 5   
                                
// La prima arriva dal server al client, e indica che entro un tempo limite
// indicato da una costante il server si aspetta la risposta (INSTR_PONG)
#define INSTR_PING 6
#define INSTR_PONG INSTR_PING

// ISTRUZIONI DAL CLIENT
// Il secondo byte dell'header sarà la dimensione della card che verrà 
// inviata successivamente dall'utente
#define INSTR_NEW_CARD 7        

// Indica una richiesta da parte dell'utente di mandare
// Tutte le card della lavagna
#define INSTR_SHOW_LAVAGNA 8    

// Ack mandato dal client una volta ricevuti tutti i peer
#define INSTR_ACK_PEERS 9
// Messaggio inviato dall'utente quando termina una card che ha in doing.
// In questo caso il secondo byte dell'header indicherà l'id della card fatta
#define INSTR_CARD_DONE 10

#define LAVAGNA_PORT 5678
#define LAVAGNA_ADDR "127.0.0.1"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "common.h"

// implementazione di htonll per il timestamp
uint64_t htonll(uint64_t num);
uint64_t ntohll(uint64_t num);

// funzione che inizializza socket listener
// ritorna il socket listener o -1 al fallimento
int init_listener(struct sockaddr_in*, uint16_t);

// manda size byte di msg via sock
// ritorna 1 se eseguita con successo 0 altrimenti
int send_msg(int, void*, size_t);

// riceve messaggio dal socket 
// (argomento 1) un messaggio di dimensione (argomento 3)
// e lo scrive nel buffer (argomento 2)
int get_msg(int, void*, size_t);

// send card al socket passato per indirizzo
int send_card(int, task_card_t *);

// socket da cui ricevere la card, dimensione card
task_card_t* recive_card(int, size_t);

#endif
