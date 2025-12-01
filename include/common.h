#ifndef STRUCTS_H
#define STRUCTS_H

#include <time.h>   
#include <stdint.h>

#define MAX_DIM_DESC 52
#define MAX_CMD_SIZE 32

#define TODO_COL 0
#define DOING_COL 1
#define DONE_COL 2

// STATI
#define STS_NOCARDS '0'

// COMANDI
#define CMD_STR_CREATE_CARD "CREATE_CARD"
#define CMD_STR_QUIT "QUIT"
#define CMD_STR_STAMPA_UTENTI_CONNESSI "SEE_CONNECTED"
// Lettere maiuscole: comandi validi per lavagna
// Lettere minuscole: comandi validi per utente 
// altri caratteri: comandi validi per entrambi
#define CMD_NOP '\0'
#define CMD_QUIT '.'
#define CMD_INVALID '-'
// lavagna
#define CMD_STAMPA_UTENTI_CONNESSI 'S'
// utente
#define CMD_CREATE_CARD 'g'

struct task_card_tipo { // 64 byte
    uint8_t id; 
    uint8_t colonna; 
    char desc[MAX_DIM_DESC];
    uint16_t utente; 
    int64_t last_modified;
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
lavagna_t* remove_from_lavagna(lavagna_t **, uint8_t);

char prompt_line(char*);

// serializza la card
// serializza card in buf, e ritorna la dimensione
size_t prepare_card(task_card_t *card, void* buf); 

// fa l'opposto di prepare_card
void unprepare_card(task_card_t *card, void* buf, size_t dim);

#endif
