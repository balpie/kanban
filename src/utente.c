#include "../include/common.h"
#include "../include/common_net.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void clear_stdin_buffer()
{
    char c;
    while((c = getchar()) != '\n'); 
}

int registra_utente(int port)
{
    int sd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in indirizzo_server;
    memset(&indirizzo_server, 0, sizeof(indirizzo_server));
    inet_pton(AF_INET, LAVAGNA_ADDR, &indirizzo_server.sin_addr.s_addr);
    indirizzo_server.sin_port = htons(LAVAGNA_PORT);
    indirizzo_server.sin_family = AF_INET;
    if(connect(sd, (struct sockaddr*)&indirizzo_server, sizeof(indirizzo_server)))
    {
        perror(">! errore connect");
        exit(-1);
    }
    int nport = htons(port);
    if(send_msg(sd, &nport, 2)) // mando al server la mia porta per registrarmi
    {
        printf(">> Registrazione effettuata con successo \n");
    }
    else
    {
        printf(">> quitting\n");
        close(sd);
        exit(-1);
    }
    return sd;
}

// FIXME Quando fai ctrl-d in mezzo a una stringa 
// da sigsev 
char* get_desc(char* buf)
{
    int count = 0;
    do{
        if(count > 0)
        {
            printf(">! la descrizone non può essere vuota\n");
        }
        printf(">> inserire la descrizione dell'attività, da terminare con a-capo. Massimo %d caratteri:\n", 
                MAX_DIM_DESC);
        if(!fgets(buf, MAX_DIM_DESC, stdin))
        {
            perror(">! errore fgets, la card non è stata creata");
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
        count++;
    }
    while(strlen(buf) == 0);
    buf = (char*)malloc((strlen(buf) + 1)*sizeof(char));
    return buf;
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
    new_card->last_modified = time(NULL);
    new_card->desc = get_desc(buf);
    if(!new_card->desc)
    {
        free(new_card);
        return NULL;
    }
    new_card->colonna = strtoul(buf, NULL, 10);
    new_card->utente = 0; // ancora non è assegnata a nessun utente
    strcpy(new_card->desc, buf);
    return new_card;
}


