#include "../include/common.h"
#include "../include/utente.h"
#include "../include/utente_net.h"
#include <stdio.h>
#include <stdlib.h>

void err_args(char* prg)
{
        printf("Utilizzo: %s <porta>\n", prg);
        printf("Il parametro <porta> deve essere un intero rappresentabile su 16 bit maggiore di 1024 diverso da 5678\n");
        exit(-1);
}

int main(int argc, char* argv[])
{
    short unsigned user_port;
    lavagna_t *lavagna = NULL;
    if(argc < 2)
    {
        err_args(argv[0]);
    }
    user_port = atoi(argv[1]);
    
    if(!user_port || user_port < 1024 || user_port == 5678)
    {
        err_args(argv[0]);
    }    
    // test
    printf("show lavagna vuota:\n");
    show_lavagna(lavagna);
    for (int i = 0; i < 4; i++)
    {
        task_card_t card;
        if(create_card(&card))
        {
            printf("Carta creata con successo!\n");
        }
        else
        {
            printf("Errore creazione carta\n");
        }
        insert_into_lavagna(&lavagna, &card);
        show_lavagna(lavagna);
    }
    show_lavagna(lavagna);
    return 0;
}
