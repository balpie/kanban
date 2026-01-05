#include "../include/common.h"
#include "../include/common_net.h"
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
// array contenente i comandi validi
const char *CMD_STR_ARR[] = {
    CMD_STR_CREATE_CARD,
    CMD_STR_QUIT,
    CMD_STR_STAMPA_UTENTI_CONNESSI,
    CMD_STR_SHOW_LAVAGNA,
    CMD_STR_CARD_DONE
};

// array contenente gli opcode dei comandi validi
const char CMD_ARR[] = {
    CMD_CREATE_CARD,
    CMD_QUIT,
    CMD_STAMPA_UTENTI_CONNESSI,
    CMD_SHOW_LAVAGNA,
    CMD_CARD_DONE
};

// Stampa rispettando una dimensione delle linee pari a size, mettendo all'inizio
// e alla fine della linea un delimiter
void print_in_box(char* str, size_t size, char delimiter)
{
    if(!str)
    {
        str = " ";
    }
    while(*str)
    {
        for(size_t j = 0; j < size; j++) 
        {
            if(j == 0 || j == size-1)
            {
                putchar(delimiter);
                continue;
            }
            if(*str)
            {
                putchar(*str);
                str++;
                continue;
            }
            putchar(' ');
        }
        putchar('\n');
    }
}

// Mostra la lavagna, assumendo ordinamento crescente rispetto alla colonna 
void show_lavagna(lavagna_t *l)
{
    putchar('\n');
    if(!l)
    {
        printf("+-----------------------+\n");
        printf("|     LAVAGNA VUOTA     |\n");
        printf("+-----------------------+\n");
        return;
    }
    printf("+---------TODO----------+\n");
    if(!(l && l->card.colonna == TODO_COL))
    {
        print_in_box(NULL, LAVAGNA_WIDTH, '|');
    }
    while(l && l->card.colonna == TODO_COL)
    {
        show_card(&(l->card));
        l = l->next;
        if(l && l->card.colonna == TODO_COL)
        {
            print_in_box(NULL, LAVAGNA_WIDTH, '|');
        }
    }
    printf("+---------DOING---------+\n");
    if(!(l && l->card.colonna == DOING_COL))
    {
        print_in_box(NULL, LAVAGNA_WIDTH, '|');
    }
    while(l && l->card.colonna == DOING_COL)
    {
        show_card(&(l->card));
        l = l->next;
        if(l && l->card.colonna == DOING_COL)
        {
            print_in_box(NULL, LAVAGNA_WIDTH, '|');
        }
    }
    printf("+---------DONE----------+\n");
    if(!(l && l->card.colonna == DONE_COL))
    {
        print_in_box(NULL, LAVAGNA_WIDTH, '|');
    }
    while(l && l->card.colonna == DONE_COL)
    {
        show_card(&(l->card));
        l = l->next;
        if(l && l->card.colonna == DONE_COL)
        {
            print_in_box(NULL, LAVAGNA_WIDTH, '|');
        }
    }
    printf("+-----------------------+\n");
    return;
}

// copia la card src in dest, assumendo che la descrizione sia già allocata
void copia_card(const task_card_t *src, task_card_t *dest)
{
    dest->id = src->id;
    dest->colonna = src->colonna;
    dest->utente = src->utente;
    dest->last_modified = src->last_modified;
    dest->desc = src->desc; 
}

// Inserimento ordinato rispetto alla colonna all'interno della lavagna
void insert_into_lavagna(lavagna_t **l, const task_card_t *card)
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

lavagna_t* extract_from_lavagna(lavagna_t **l, uint8_t id)
{
    lavagna_t* iter = *l;
    lavagna_t* prec = NULL;
    while(iter && iter->card.id != id)
    {
        prec = iter;
        iter = iter->next;
    }
    if(!iter) //arrivato in fondo senza trovare la card che cercavo
    { 
        return NULL;
    }
    if(!prec) //rimozione in testa
    {
        *l = (*l)->next;
        return iter;
    }
    // rimozione altrove
    prec->next = iter->next;
    return iter;
}

// Dealloca la lavagna. E' poi necessario settare dal chiamante
// ll = NULL per evitare undefine behaviour
void libera_lavagna(lavagna_t* ll)
{
    if(!ll)
        return;
    lavagna_t* tmp = ll->next;
    free(ll->card.desc);
    free(ll);
    libera_lavagna(tmp);
}

// stampa la stringa passata rispettando un box laterale di dimensione size, 
// e mette delimiter prima e dopo essere andato a capo
void show_card(task_card_t *cc)
{
    char timebuf[9];
    char other[MAX_DIM_DESC + 5];
    struct tm *lastmod = localtime(&cc->last_modified);
    strftime(timebuf, 9, "%H:%M:%S", lastmod);
    sprintf(other, "card: %u", cc->id);
    print_in_box(other, LAVAGNA_WIDTH, '|');
    if(VALID_PORT(cc->utente))
    {
        sprintf(other, "user: %u", cc->utente);
    }
    else
    {
        sprintf(other, "user: NO_USR");
    }
    print_in_box(other, LAVAGNA_WIDTH, '|');
    sprintf(other, "time: %s", timebuf);
    print_in_box(other, LAVAGNA_WIDTH, '|');
    sprintf(other, "desc: %s   ", cc->desc);
    print_in_box(other, LAVAGNA_WIDTH, '|');
}

// rende str maiuscola. 
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

// valuta comandi e ritorna carattere associato al comando
char eval_cmdbuf(char* cmd) 
{
    char* nl = strchr(cmd, '\n');
    if(nl)
    {
        *nl = '\0';
    }
    if(!strlen(cmd))
    {
        return CMD_NOP;
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
                            // oppure il prefisso è comune a più di un comando
}

char prompt_line(char* content)
{
    char cmdbuf[MAX_CMD_SIZE];
    printf("%s> ", content);
    if(!fgets(cmdbuf, MAX_CMD_SIZE, stdin)) // caso ctrl-d o errore
    {
        LOG( "\nCOMMON(prompt_line): fgets ha ritornato 0, fileno(stdin): %d\n errore:\n",
                fileno(stdin));
        if(ferror(stdin))
            perror(NULL);
        printf("\n");
        return CMD_QUIT;
    }
    return eval_cmdbuf(cmdbuf);
}

// serializzazione e hton dei membri numerici con sizeof > 1
// la funzione assume che buf sia stato allocato dal chiamante
size_t prepare_card(task_card_t *card, void* buf)
{
    // max 255 byte (escludendo \0)
    size_t dim_desc = strlen(card->desc);
    uint64_t timestamp = htonll(card->last_modified);
    uint16_t usr = htons(card->utente);
    // id
    memcpy(buf, (void*)&card->id, sizeof(card->id)); 
    buf = (char*)buf + sizeof(card->id); // 1
    // colonna
    memcpy(buf, (void*)&card->colonna, sizeof(card->colonna)); 
    buf = (char*)buf + sizeof(card->colonna); // 1 
    // utente
    memcpy(buf, (void*)&usr, sizeof(card->utente)); 
    buf = (char*)buf + sizeof(card->utente); // 2
    // timestamp
    memcpy(buf, (void*)&timestamp, sizeof(card->last_modified)); 
    buf = (char*)buf + sizeof(card->last_modified); // 8
    // descrizione
    memcpy(buf, (void*)card->desc, dim_desc);
    return dim_desc;
}

// dim è la dimensione della descrizione, escluso \0
void unprepare_card(task_card_t *card, void* buf, size_t dim_desc)
{
    // id
    memcpy((void*)&card->id, buf, 1);
    buf = (char*)buf + sizeof(card->id);
    // colonna
    memcpy((void*)&card->colonna, buf, 1);
    buf = (char*)buf + sizeof(card->colonna);
    // utente
    memcpy((void*)&card->utente, buf, sizeof(card->utente));
    card->utente = ntohs(card->utente);
    buf = (char*)buf + sizeof(card->utente);
    // timestamp
    memcpy((void*)&card->last_modified, buf, sizeof(card->last_modified));
    card->last_modified = ntohll(card->last_modified);
    buf = (char*)buf + sizeof(card->last_modified);

    card->desc = (char*)malloc((dim_desc + 1)*(sizeof(char)));
    memcpy(card->desc, buf, dim_desc);
    card->desc[dim_desc] = '\0';
}
