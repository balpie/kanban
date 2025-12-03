#ifndef LAVAGNA_H
#define LAVAGNA_H
#include "lavagna_net.h"

// Massimo 100 processi client serviti contemporaneamente
#define MAX_SERVER_PROCS 100


struct client_info{
    int socket;
    uint32_t addr; // network order
};

// TODO
// Devo associare un puntatore a card, tipo "da mandare", ad ogni thread di ogni connessione attiva
// Quando c'è da associare una card a un utente, il thread che "se ne accorge"
// toglie NULL da questa struttura per tutti i thread delle connessioni attualmente attive, 
// e quando un thread trova nel proprio puntatore un valore non null, manda la card
// al suo utente, e tutto il resto che deve mandare.
struct server_status{
    int n_connessioni;
    int status; // indica se c
    pthread_rwlock_t rwlock;
};

// Variabili globali
extern connection_l lista_connessioni;
extern int sock_listener; // in modo da poter terminare dai thread
extern lavagna_t *lavagna;
extern struct server_status status;
// THREADS
void* prompt_cycle(void *);

void* serv_client(void* cl_info);
// STAMPA_UTENTI_CONNESS
// Comando che permette di visualizzare il 
// contenuto della linked list con all'interno le connessioni
void stampa_utenti_connessi(connection_l_e *head);
  
void cleanup(int, connection_l_e**); 
// MOVE_CARD:
// Sposta una card da una colonna ad un'altra
// Viene eseguita ogni volta che viene ricevuta 
// un'informazione da parte di un utente

// SHOW_LAVAGNA:
// Viene mostrata la lavagna, con le card assegnate ognuna alla giusta colonna

// SEND_USER_LIST:
// Manda la lista delle porte degli utenti 

// PING_USER:
// Invia un messaggio periodicamente agli utenti con una card in doing, 
// se questi non rispondono entro un timeout con pong_lavagna vengono considerati non connessi
// e la/le relative task finiscono in todo

// AVALIABLE_CARD:
// Comunica a tutti gli utenti la prima carta disponibile nella colonna
// todo, e la lista degli utenti presenti
// escluso l'utente a cui viene mandato.


#endif
