#include "../include/common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
  
void clear_stdin_buffer()
{
    char c;
    while((c = getchar()) != '\n'); 
}

// Ritorna 1 in caso di successo, 0 altrimenti
task_card_t *create_card()
{
    task_card_t *new_card = (task_card_t*)malloc(sizeof(task_card_t));
    char buf[MAX_DIM_DESC];
    printf(">> inserire id (0 <= id < 256): ");
    if(!fgets(buf, 4, stdin)) // 3 cifre + \n
    {
        perror(">! errore fgets, la card non è stata creata");
        free(new_card);
        return NULL;
    }

    char* endlptr = strchr(buf, '\n');
    if(!endlptr)
    { 
        clear_stdin_buffer();
    }
    else
    {
        *endlptr = '\0';
    }
    new_card->id = strtoul(buf, NULL, 10);
    printf("\t0: To Do\n");
    printf("\t1: Doing\n");
    printf("\t2: Done\n");
    printf(">> inserire colonna: ");
    if(!fgets(buf, 2, stdin)) // 1 cifra + \n
    {
        perror(">! errore fgets, la card non è stata creata");
        free(new_card);
        return NULL;
    }
    buf[1] = '\0'; // Sicuramente ho un a-capo nel buffer
    clear_stdin_buffer();
    
    new_card->colonna = strtoul(buf, NULL, 10);
    new_card->utente = 0; // ancora non è assegnata a nessun utente
    printf(">> inserire la descrizione dell'attività, da terminare con a-capo. Massimo %d caratteri:\n", 
            MAX_DIM_DESC);
    if(!fgets(buf, MAX_DIM_DESC, stdin))
    {
        perror(">! errore fgets, la card non è stata creata");
        free(new_card);
        return NULL;
    }
    endlptr = strchr(buf, '\n');
    if(!endlptr)
    { 
        clear_stdin_buffer();
    }
    else
    {
        *endlptr = '\0';
    }
    new_card->last_modified = time(NULL);
    new_card->desc = (char*)malloc((strlen(buf) + 1)*sizeof(char));
    strcpy(new_card->desc, buf);
    return new_card;
}
