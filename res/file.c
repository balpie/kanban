#include <stdio.h>
#include <time.h>
#include <stdint.h>
#include <stdlib.h>
#define FILENAME "cards"

struct task_card_tipo { 
    uint8_t id; 
    uint8_t colonna; 
    uint16_t utente; 
    int64_t last_modified; 
    // Allocata dinamicamente
    char *desc; 
};
typedef struct task_card_tipo task_card_t;

char* get_desc(FILE *cfile);
void print_card(task_card_t *cc);
task_card_t *parse_card(FILE *cfile);

int main()
{
    FILE *cfile = fopen(FILENAME, "r");
    char str[5];
    task_card_t* card;
    while((card = parse_card(cfile))){
        print_card(card);
        printf("time(NULL): %lu\n", time(NULL));
    }
    return 0;
}

void print_card(task_card_t *cc)
{
    printf("id: %u, col: %u, usr: %u, last_modified: %lu, desc: %s\n"
            , cc->id, cc->colonna, cc->utente, cc->last_modified, cc->desc);
}

task_card_t *parse_card(FILE *cfile)
{
    task_card_t *cc = (task_card_t *)malloc(sizeof(task_card_t));
    if(fscanf(cfile, "%hhu\n%hhu\n%hu\n%lu\n", &cc->id, &cc->colonna, &cc->utente, &cc->last_modified) == EOF)
    {
        free(cc);
        return NULL;
    }
    cc->desc = get_desc(cfile);
    return cc;
}


char* get_desc(FILE *cfile)
{
    // Conto quanto è grande la descrizione
    long inizio = ftell(cfile);
    int c;
    do
    {
        c = getc(cfile);
    }while(c != EOF && c != '\n');
    long fine = ftell(cfile);
    unsigned len = fine - inizio;
    if(len <= 1)
    {
        printf("descrizione vuota\n");
    }
    printf("dimensione stringa: %u\n", len);
    if(fseek(cfile, inizio, SEEK_SET))
    {
        printf("Errore fseek\n");
        exit(-1);
    }
    char* desc = (char*)malloc(len);
    int offs = 0;
    do
    {
        desc[offs++] = getc(cfile);
    }while(desc[offs-1] != '\n' && desc[offs-1] != EOF);
    desc[len-1] = '\0';
    return desc;
}
