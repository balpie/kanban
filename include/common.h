#ifndef STRUCTS_H
#define STRUCTS_H

#include <time.h>   
#include <stdint.h>

#define MAX_CMD_SIZE 32

#define TODO_COL 0
#define DOING_COL 1
#define DONE_COL 2

// COMANDI
// I comandi sono rappresentazioni interne di ciò che deve essere fatto, a 
// differenza delle istruzioni (INSTR_) che indicano un'operazione che richiede
// la collaborazione della controparte per essere portata a termine
#define CMD_STR_CREATE_CARD "CREATE_CARD"
#define CMD_STR_QUIT "QUIT"
#define CMD_STR_STAMPA_UTENTI_CONNESSI "SEE_CONNECTED"
#define CMD_STR_SHOW_LAVAGNA "SHOW_LAVAGNA"
#define CMD_STR_CARD_DONE "CARD_DONE"

#define CMD_NOP '\0'
#define CMD_QUIT '.'
#define CMD_INVALID '-'
#define CMD_SHOW_LAVAGNA '#'
// lavagna
#define CMD_STAMPA_UTENTI_CONNESSI 'S'
// utente
#define CMD_CREATE_CARD 'g'
#define CMD_CARD_DONE 'd'

// valore di porta considerato come nessun utente
// In realtà andrebbe bene qualsiasi intero <= 5678
#define NO_USR 1023U

#define MAX_DIM_DESC 256
// dimensione tale che una task card occupi al più
// 256 byte, di cui uno per '\0', in modo da poter
// esprimere la dimensione dei messaggi con un singolo byte

// error logs

// I logfile sono pensati per essere visti da terminale con cat, in modo che
// DBG, ERR, e TST siano colorati. Altrimenti si vedranno dei caratteri strani
#define LOG(...) fprintf(stderr, "[log] " __VA_ARGS__)
// Log di errore di colore rosso su terminale
#define ERR(fmt, ...)\
    fprintf(stderr, "\033[31;10;10m[err] "fmt"\033[0m", ##__VA_ARGS__)
// stampe debug di colore blu su terminale
#define DBG(fmt, ...)\
    fprintf(stderr, "\033[34;10;10m[dbg] "fmt"\033[0m", ##__VA_ARGS__)
// Stampe test di colore verde su terminale
#define TST(fmt, ...)\
    fprintf(stderr, "\033[32;10;10m[log] "fmt"\033[0m", ##__VA_ARGS__)
// Utility
#define VALID_PORT(pp) ((pp > 5678) ? (1) : (0))

// Dimensione orizzontale della lavagna su terminale (inclusi delimitatori)
#define LAVAGNA_WIDTH 30

struct task_card_tipo { 
    uint8_t id; 
    uint8_t colonna; 
    uint16_t utente; 
    int64_t last_modified; 
    // Allocata dinamicamente
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

// mostra intera lavagna
void show_lavagna(lavagna_t *l);

// inserimento ordinato rispetto alla colonna
void insert_into_lavagna(lavagna_t **l, const task_card_t *card);

// rimuove la task id dalla lavagna. Utilizzato per move_card
lavagna_t* extract_from_lavagna(lavagna_t **, uint8_t);

char prompt_line(char*);

// serializza card in buf, e ritorna la dimensione
size_t prepare_card(task_card_t *card, void* buf); 

// fa l'opposto di prepare_card
void unprepare_card(task_card_t *card, void* buf, size_t dim);

// libera la memoria allocata dinamicamente della lavagna.
// se la lavagna va utilizzata ancora bisogna assegnarvi NULL per
// mantenerla consistente
void libera_lavagna(lavagna_t*);

// Copia il contenuto di src in dest. Non alloca una nuova descrizione, ma 
// assume che la descrizione di src sia già allocata e cambia solo il puntatore
void copia_card(const task_card_t *, task_card_t *);

#endif
