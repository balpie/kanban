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
int create_card(task_card_t *new_card)
{
    char buf[MAX_DIM_DESC];
    printf("Inserire id card (0 <= id < 256): ");
    if(!fgets(buf, 4, stdin)) // 3 cifre + \n
    {
        perror("Errore fgets");
        // TODO error handle
    }
    char* endlptr= strchr(buf, '\n');
    if(!endlptr)
    { 
        clear_stdin_buffer();
    }
    else
    {
        *endlptr = '\0';
    }
    new_card->id = strtoul(buf, NULL, 10);
    printf("0: To Do\n");
    printf("1: Doing\n");
    printf("2: Done\n");
    printf("Inserire colonna: ");
    if(!fgets(buf, 2, stdin)) // 1 cifra + \n
    {
        perror("Errore fgets");
        return 0;
    }
    buf[1] = '\0';
    clear_stdin_buffer();
    // Sicuramente ho un a-capo nel buffer
    new_card->colonna = strtoul(buf, NULL, 10);
    printf("Inserire id utente (porta): ");
    if(!fgets(buf, 5, stdin)) // 4 cifra + \n
    {
        perror("Errore fgets");
        return 0;
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
    new_card->utente = strtoul(buf, NULL, 10);
    if(new_card->utente < 1024)
    {
        printf("Porta inaccettabile\n");
        return 0;
    }
    printf("Inserire la descrizione dell'attività, da terminare con a-capo. Massimo 51 caratteri:\n");
    if(!fgets(buf, MAX_DIM_DESC, stdin))
    {
        perror("Errore fgets");
        return 0;
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
    strcpy(new_card->desc, buf);
    return 1;
}
