#ifndef UTENTE_H
#define UTENTE_H
#include "../include/common.h"
#include "../include/common_net.h"
#include <pthread.h>
#include <stdatomic.h>

#define MAX_QUEUE_CMD 10

#define COMMON_LOGFILE_NAME "log/log_usr"

  
// Coda circolare dei comandi da eseguire
extern char cmd_queue[MAX_QUEUE_CMD];
// quando coincidono testa e coda, la lista è vuota
// quando la testa è subito dietro la coda, la lista è piena
extern int cmd_head;
extern int cmd_tail;

// Carta creata dal prompt thread
extern task_card_t *created;
// carta attualmente in doing. NULL se non presente
extern lavagna_t *doing;
extern pthread_mutex_t created_m;

// lavagna corrente. L'utente assuma che non sia mai up to date: 
// ogni volta che la vuole vedere chiede alla lavagna le card attuali e le aggiorna
extern lavagna_t* lavagna;

extern int listener;
// Funzione del thread che gestisce il prompt. 
// Inserisce comandi nella cmd_queue, in modo che 
// il main thread li possa eseguire, se hanno bisogno di comunicare con la lavagna
void* prompt_cycle_function(void*);
// CREATE_CARD:
// Crea carta (con id, colonna, e descrizione prese da tastiera)
task_card_t* create_card(); 
  
// connettiti ed effettua la registrazione al server
int registra_utente(int);
// SHOW_LAVAGNA:
// Viene mostrata la lavagna, con le card assegnate ognuna alla giusta colonna

// PONG_LAVAGNA:
// Messaggio di risposta a PING_USER

// CHOSE_USER
// Messaggio da inviare tra utenti per decidere chi prenderà la card
// randomizzare il costo, card a costo minore, a parità di costo porta minore

// ACK_CARD
// L'utente a cui viene assegnata la card lo comunica alla lavagna

// CARD_DONE
// L'utente comunica alla lavagna la terminazione dell'attività
#endif
