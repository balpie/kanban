#ifndef UTENTE_H
#define UTENTE_H
#include "../include/common.h"
#include "../include/common_net.h"
#include <pthread.h>
#include <stdatomic.h>

#define MAX_QUEUE_CMD 10

#define COMMON_LOGFILE_NAME "log/log_usr"

// Tempo in cui una carta rimane in doing in secondi.
// Una volta scaduto la card viene mandata come done a lavagna
#define MAX_TIME_DOING 5
  
// Coda circolare dei comandi da eseguire
extern char cmd_queue[MAX_QUEUE_CMD];
// quando coincidono testa e coda, la lista è vuota
// quando la testa è subito dietro la coda, la lista è piena
extern int cmd_head;
extern int cmd_tail;
extern pthread_mutex_t cmd_queue_m;

// Carta creata dal prompt thread
extern task_card_t *created;
// carta attualmente in doing. NULL se non presente
extern lavagna_t *doing;
extern pthread_mutex_t created_m;
// timestamp istante in cui la card attualmente in doing è stata presa in carico
extern time_t doing_timestamp; 

extern char prompt_msg[12];
extern char user_prompt[13]; // utentexxxxxx\0

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


// Disconnette l'utente dalla lavagna e termina(da passare socket del server)
void disconnect(int);

// CARD_DONE
// L'utente comunica alla lavagna la terminazione dell'attività
// terminerà l'attività in testa alla lista passata
// per argomento, e verrà rimossa dalla relativa lista
// ritorna 0 in caso di fallimento, 1 altrimenti
int card_done(int, lavagna_t **);

// controlla il tempo attuale e lo confronta con timestamp.
// Se la differenza è maggiore di MAX_TIME_DOING manda card done alla lavagna
// ritorna 1 se ha mandato la card, 0 altrimenti
int send_if_done(int); //TODO 
#endif
