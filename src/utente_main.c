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
        printf("Utilizzo: %s <porta>\n", prg);
        printf("Il parametro <porta> deve essere un intero rappresentabile su 16 bit maggiore di 5678\n");
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
        printf("Comando vuoto per uscire\n");
        char c;
        do
        {
            c = prompt_line("prova prompt utente");
            switch(c)
            {
                case CMD_NOP:
                    break;
                case CMD_INVALID:
                    printf("Comando inesistente\n");
                    break;
                case CMD_CREATE_CARD:
                    create_card(); 
                    break;
            }
        }while(c != CMD_QUIT);
    }
    close(sd);
    return 0;
}
