#include "../include/common.h"
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>

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

void show_card(task_card_t *cc)
{
    char buf[21];
    struct tm *lastmod = localtime(&cc->last_modified);
    strftime(buf, 21, "%D %H:%m:%S", lastmod);
    printf("Id card: %u\n", cc->id);
    printf("Id utente: %u\n", cc->utente);
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

char prompt_line(char* content)
{
    char cmdbuf[MAX_CMD_SIZE];
    printf("%s> ", content);
    fgets(cmdbuf, MAX_CMD_SIZE, stdin);
    char* nl = strchr(cmdbuf, '\n');
    if(nl)
    {
        *nl = '\0';
    }
    // Forse meglio fare funzione char eval_cmdbuf(char*)?
    to_upper_case(cmdbuf);
    if(strlen(cmdbuf) == 0)
    {
        return CMD_NOP;
    }
    if(strcmp(cmdbuf, CMD_STR_CREATE_CARD) == 0)
    {
        return CMD_CREATE_CARD;
    }
    if(strcmp(cmdbuf, CMD_STR_QUIT) == 0)
    {
        return CMD_QUIT;
    }
    if(strcmp(cmdbuf, CMD_STR_STAMPA_UTENTI_CONNESSI) == 0)
    {
        return CMD_STAMPA_UTENTI_CONNESSI;
    }
    return CMD_INVALID;
}

