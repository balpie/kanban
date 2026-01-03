#ifndef LAVAGNA_H
#define LAVAGNA_H
#include "lavagna_net.h"

// nome logfile in caso non venga passato argomento -d
#define LOGFILE_NAME "log/log_lav"

// Tempi in secondi
// attesa tra una richiesta di ping e un altra
#define TIME_PING 2U

// Massimo delay pong
#define TIME_PONG_MAX_DELAY 3U

// Massimi processi serviti contemporaneamente
#define MAX_SERVER_PROCS 256

struct client_info{
    int socket;
    uint32_t addr; // network order
};

struct server_status{
    int n_connessioni;
    uint8_t status; 
    // mutex per accesso a status 
    pthread_mutex_t m;      
    // condition variable per attesa
    pthread_cond_t cv;      
    // indica il numero di utenti a cui è stata già mandata la card (valido se 
    // c'è una card da mandare e abbastanza utenti)
    uint8_t sent;           
    // indica il numero di utenti collegati alla lavagna nel momento
    // in cui una card viene "etichettata come tosend"
    uint8_t total;          
    // variabile che indica, nel caso in cui gli utenti siano in fase p2p, quanti
    // di questi abbiano mandato il vincitore
    uint8_t winner_arrived; 
};

// Variabili globali
extern connection_l lista_connessioni;
// Socket del listener, in modo che tutti i thread possano terminare 
// ripulendo tutto
extern int sock_listener; 

extern lavagna_t *lavagna;
extern pthread_rwlock_t m_lavagna;

extern struct server_status status;

extern struct timeval timeout_recv; 

// THREADS
void* prompt_cycle(void *);

void* serv_client(void* cl_info);
// STAMPA_UTENTI_CONNESS
// Comando che permette di visualizzare il 
// contenuto della linked list con all'interno le connessioni
void stampa_utenti_connessi(connection_l_e *head);

// Passato listener socket e lista di connessioni, libera la memoria delle 
// connessioni allocate e chiude tutti i socket
void cleanup(int, connection_l_e**); 

// manda tutte le card via socket
void send_lavagna(int, lavagna_t*);

#endif
