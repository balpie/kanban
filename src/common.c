#include "../include/common.h"
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>

const char *CMD_STR_ARR[] = {
    CMD_STR_CREATE_CARD,
    CMD_STR_QUIT,
    CMD_STR_STAMPA_UTENTI_CONNESSI
};

const char CMD_ARR[] = {
    CMD_CREATE_CARD,
    CMD_QUIT,
    CMD_STAMPA_UTENTI_CONNESSI
};

void show_lavagna(lavagna_t *l)
{
    printf("-----TO DO-----");
    while(l && l->card.colonna == TODO_COL)
    {
        show_card(&(l->card));
        l = l->next;
    }
    printf("-----DOING-----");
    while(l && l->card.colonna == DOING_COL)
    {
        show_card(&(l->card));
        l = l->next;
    }
    printf("-----DONE-----");
    while(l && l->card.colonna == DONE_COL)
    {
        show_card(&(l->card));
        l = l->next;
    }
    return;
}

void copia_card(task_card_t *src, task_card_t *dest)
{
    dest->id = src->id;
    dest->colonna = src->colonna;
    dest->utente = src->utente;
    dest->last_modified = src->last_modified;
    for(int i = 0; i < MAX_DIM_DESC; i++)
    {
        dest->desc[i] = src->desc[i];
    }
}

// Inserimento ordinato rispetto alla colonna all'interno della lavagna
void insert_into_lavagna(lavagna_t **l, task_card_t *card)
{
    lavagna_t* new = (lavagna_t*)malloc(sizeof(lavagna_t));
    copia_card(card, &(new->card));
    if(!*l)
    {
        (*l) = new;
        new->next = NULL;
        return;
    }
    lavagna_t *iter = *l;
    lavagna_t *prec = NULL;
    while(iter && iter->card.colonna < card->colonna)
    {
        prec = iter;
        iter = iter->next;
    }
    if(!prec) // inserimento in testa
    {
        new->next = iter;
        *l = new;
        return;
    }
    prec->next = new;
    new->next = iter;
}

lavagna_t* remove_from_lavagna(lavagna_t **l, uint8_t id)
{
    lavagna_t* iter = *l;
    lavagna_t* prec = NULL;
    while(iter && iter->card.id != id)
    {
        prec = iter;
        iter = iter->next;
    }
    if(!iter) // arrivato in fondo senza trovare la card che cercavo
    { 
        return NULL;
    }
    if(!prec) // rimozione in testa
    {
        *l = (*l)->next;
        return iter;
    }
    // rimozione altrove
    prec->next = iter->next;
    return iter;
}


void show_card(task_card_t *cc)
{
    char buf[21];
    struct tm *lastmod = localtime(&cc->last_modified);
    strftime(buf, 21, "%D %H:%m:%S", lastmod);
    printf("Id card: %u\n", cc->id);
    printf("Id utente: "); 
    (cc->utente) ? printf("%u\n", cc->utente) : printf("card non ancora assegnata\n");
    printf("Ultima modifica: %s\n", buf);
    printf("Descrizione card:\n%s\n", cc->desc);
}

void to_upper_case(char* str) 
{
    for(char* ptr = str; *ptr != '\0'; ptr++)
    {
        if(*ptr >= 'a' && *ptr <= 'z')
        {
            *ptr -= 'a' - 'A';
        }
    }
}
  
// valuta comandi non vuoti e ritorna carattere associato al comando
char eval_cmdbuf(char* cmd) 
{
    char* nl = strchr(cmd, '\n');
    if(nl)
    {
        *nl = '\0';
    }
    to_upper_case(cmd);
    int count_matches = 0;
    int match = -1;
    for(int i = 0; i < sizeof(CMD_STR_ARR)/sizeof(CMD_STR_ARR[0]); i++)
    { // confronto la mia stringa con tutti i possibili comandi
        int dim_cmd = strlen(cmd);
        int dim_curr = strlen(CMD_STR_ARR[i]);
        if(dim_cmd > dim_curr) 
            continue;
        if(strncmp(cmd, CMD_STR_ARR[i], dim_cmd) == 0)
        {
            count_matches++;
            match = i;
        }
    }
    if(count_matches == 1) // se il match è uno (solo)
        return CMD_ARR[match]; // ritorno il comando
    else
        return CMD_INVALID; // altrimenti c'è un errore di sintassi
}

char prompt_line(char* content)
{
    char cmdbuf[MAX_CMD_SIZE];
    printf("%s> ", content);
    if(!fgets(cmdbuf, MAX_CMD_SIZE, stdin)) // caso ctrl-d o errore
    {
        printf("\n");
        return CMD_QUIT;
    }
    return eval_cmdbuf(cmdbuf);
}

