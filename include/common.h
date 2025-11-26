#ifndef STRUCTS_H
#define STRUCTS_H

#include <time.h>   
#include <stdint.h>

#define MAX_DIM_DESC 52
#define TODO_COL 0
#define DOING_COL 1
#define DONE_COL 2

struct task_card_tipo { // 64 byte
    uint8_t id; 
    uint8_t colonna; 
    uint16_t utente; 
    time_t last_modified;
    char desc[MAX_DIM_DESC];
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

void insert_into_lavagna(lavagna_t **l, task_card_t *card);
#endif
