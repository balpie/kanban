#ifndef LAVAGNA_H
#define LAVAGNA_H
#include "lavagna_net.h"

#define LOGFILE_NAME "log/log_lav"

#define TIME_PING 90U
#define TIME_PONG_MAX_DELAY 30U

#define MAX_SERVER_PROCS 256

struct client_info{
    int socket;
    uint32_t addr; // network order
};

struct server_status{
    int n_connessioni;
    uint8_t status; 
    pthread_mutex_t m;
    pthread_cond_t cv;
    uint8_t sent;  // indica il numero di utenti a cui è stata già mandata la card (valido se 
                   // c'è una card da mandare e abbastanza utenti)
    uint8_t total; // indica il numero di utenti collegati alla lavagna nel momento
                   // in cui una card viene "etichettata come tosend"
    uint8_t winner_arrived; // variabile che indica, nel caso in cui gli utenti siano in fase p2p, quanti
                            // di questi abbiano mandato il vincitore
};

// Variabili globali
extern connection_l lista_connessioni;
extern int sock_listener; // in modo da poter terminare dai thread
extern lavagna_t *lavagna;
extern pthread_rwlock_t m_lavagna;
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

// manda tutte le card via socket
void send_lavagna(int, lavagna_t*);
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
