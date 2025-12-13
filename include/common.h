#ifndef STRUCTS_H
#define STRUCTS_H

#include <time.h>   
#include <stdint.h>

#define MAX_CMD_SIZE 32

#define TODO_COL 0
#define DOING_COL 1
#define DONE_COL 2

// COMANDI
#define CMD_STR_CREATE_CARD "CREATE_CARD"
#define CMD_STR_QUIT "QUIT"
#define CMD_STR_STAMPA_UTENTI_CONNESSI "SEE_CONNECTED"
#define CMD_STR_SHOW_LAVAGNA "SHOW_LAVAGNA"
// Lettere maiuscole: comandi validi per lavagna
// Lettere minuscole: comandi validi per utente 
// altri caratteri: comandi validi per entrambi
#define CMD_NOP '\0'
#define CMD_QUIT '.'
#define CMD_INVALID '-'
#define CMD_SHOW_LAVAGNA '#'
// lavagna
#define CMD_STAMPA_UTENTI_CONNESSI 'S'
// utente
#define CMD_CREATE_CARD 'g'

// valore di porta considerato come nessun utente
#define NO_USR 1023U

#define MAX_DIM_DESC 256
// dimensione tale che una task card occupi al più
// 256 byte, di cui uno per '\0', in modo da poter
// esprimere la dimensione dei messaggi con un singolo byte

struct task_card_tipo { 
    uint8_t id; 
    uint8_t colonna; 
    uint16_t utente; 
    int64_t last_modified;
    char *desc;
};
typedef struct task_card_tipo task_card_t;

typedef struct lavagna_tipo lavagna_t;
struct lavagna_tipo{
    task_card_t card;
    lavagna_t* next;
};

// mostra singola card
void show_card(task_card_t *);

void show_lavagna(lavagna_t *l);

// ordinata rispetto alla colonna
void insert_into_lavagna(lavagna_t **l, task_card_t *card);

// rimuove la task id dalla lavagna. Utile per riordinarla
lavagna_t* extract_from_lavagna(lavagna_t **, uint8_t);

char prompt_line(char*);

// serializza la card
// serializza card in buf, e ritorna la dimensione
size_t prepare_card(task_card_t *card, void* buf); 

// fa l'opposto di prepare_card
void unprepare_card(task_card_t *card, void* buf, size_t dim);

// libera la memoria allocata dinamicamente della lavagna.
// se la lavagna va utilizzata ancora bisogna assegnarvi NULL per
// mantenerla consistente
void libera_lavagna(lavagna_t*);

#endif
