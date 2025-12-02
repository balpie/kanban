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
    int sd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in indirizzo_server;
    memset(&indirizzo_server, 0, sizeof(indirizzo_server));
    inet_pton(AF_INET, LAVAGNA_ADDR, &indirizzo_server.sin_addr.s_addr);
    indirizzo_server.sin_port = htons(LAVAGNA_PORT);
    indirizzo_server.sin_family = AF_INET;
    if(connect(sd, (struct sockaddr*)&indirizzo_server, sizeof(indirizzo_server)))
    {
        perror("errore connect");
    }
    else
    {
        printf(">> Registrazione al server con porta %u...\n", user_port);
        int nport = htons(user_port);
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
                    // !! Create card la alloca nello heap
                    task_card_t* cc = create_card(); 
                    show_card(cc);
                    size_t net_card = sizeof(*cc) - sizeof(cc->desc) + strlen(cc->desc);
                    char buffer[net_card]; 
                    unsigned dim = prepare_card(cc, buffer);
                    char instr_to_server[2]; 
                    char instr_from_server[2];
                    get_msg(sd, instr_from_server, 2);
                    instr_to_server[1] = dim;
                    send_msg(sd, instr_to_server, 2);
                    send_msg(sd, buffer, dim + sizeof(*cc) - sizeof(cc->desc));
                    free(cc->desc); // libero la descrizione, anc'essa allocata nello heap
                    free(cc);
                    break;
            }
        }while(c != CMD_QUIT);
    }
    close(sd);
    return 0;
}
