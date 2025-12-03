#include "../include/common.h"
#include "../include/common_net.h"
#include "../include/utente.h"
#include "../include/utente_net.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void err_args(char* prg)
{
        printf(">! utilizzo: %s <porta>\n", prg);
        printf(">! il parametro <porta> deve essere un intero rappresentabile su 16 bit maggiore di 5678\n");
        exit(-1);
}

int main(int argc, char* argv[])
{
    short unsigned user_port;
    if(argc < 2)
    {
        err_args(argv[0]);
    }
    user_port = atoi(argv[1]);
    
    if(!user_port || user_port < 5679)
    {
        err_args(argv[0]);
    }    
    int server_sock = registra_utente(user_port);
    printf(">> Registrazione al server con porta %u...\n", user_port);
    char c;
    do
    {
        // TODO Fai a meno della dichiarazione di variabili all'interno dello switch
        //      in oltre se c è un comando per lavagna bisogna fare come per CMD_INVALID
        c = prompt_line("utente");
        switch(c)
        {
            case CMD_NOP:
                break;
            case CMD_INVALID:
                printf(">! Comando inesistente\n");
                break;
            case CMD_CREATE_CARD:
                task_card_t* cc = create_card();  // crea card
                send_card(server_sock, cc); // manda card al server
                free(cc->desc); // libero la descrizione, anc'essa allocata nello heap
                free(cc); // libero la card
                break;
        }
    }while(c != CMD_QUIT);
    close(server_sock);
    return 0;
}
