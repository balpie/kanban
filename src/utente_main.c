#include "../include/common.h"
#include "../include/common_net.h"
#include "../include/utente.h"
#include "../include/utente_net.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

lavagna_t *lavagna = NULL;
// coda circolare dei comandi
char cmd_queue[MAX_QUEUE_CMD];
task_card_t *created = NULL;
pthread_mutex_t created_m;
int cmd_head = 0;
int cmd_tail = 0;

void err_args(char* prg)
{
        printf(">! utilizzo: %s <porta>\n", prg);
        printf(">! il parametro <porta> deve essere un intero rappresentabile su 16 bit maggiore di 5678\n");
        exit(-1);
}

int main(int argc, char* argv[])
{
    memset(cmd_queue, 0, MAX_QUEUE_CMD);
    short unsigned user_port;
    pthread_mutex_init(&created_m, NULL);
    if(argc < 2)
    {
        err_args(argv[0]);
    }
    user_port = atoi(argv[1]);
    pthread_t prompt_cycle;
    if(!user_port || user_port < 5679)
    {
        err_args(argv[0]);
    }    
    printf(">> Registrazione al server con porta %u...\n", user_port);
    int server_sock = registra_utente(user_port);
    int self_info[2] = {user_port, server_sock};

    pthread_create(&prompt_cycle, NULL, prompt_cycle_function, self_info);
    pthread_detach(prompt_cycle);
        
    for(;;) // Ciclo di comunicazione con server
    {
        char instr_to_server[2];
        char instr_from_server[2];
        get_msg(server_sock, instr_from_server, 2);
        // IF instr_from_server[0] == INSTR_NOP
        // ovvero, il client decide cosa fare solo nel caso in cui il server
        // non ha nulla da fare
        char c;
        if(cmd_tail == cmd_head) // caso in cui non ho comandi da eseguire
        {
            instr_to_server[0] = INSTR_NOP;
            send_msg(server_sock, instr_to_server, 2); // comunico al server che anche il client
                                                       // non ha nulla da fare
            sleep(1);
            continue;
        }
        c = cmd_queue[cmd_tail];
        cmd_tail = (cmd_tail + 1) % MAX_QUEUE_CMD;
        fprintf(stderr ,"[dbg] main: comando arrivato: %c\n", c);
        switch(c)
        {
            case CMD_CREATE_CARD:
                pthread_mutex_lock(&created_m);
                printf("\n>> mandando card appena creata...\n");

                // posso mandare la card
                send_card(server_sock, created); // manda card al server
                free(created->desc); // libero la descrizione, anc'essa allocata nello heap
                free(created); // libero la card
                pthread_mutex_unlock(&created_m);
                printf("\n>> carta mandata!\n");
                printf("utente> ");
                fflush(stdout);
                break;
            case CMD_SHOW_LAVAGNA:
                fprintf(stderr, "[dbg]: arrivato comando SHOW_LAVAGNA\n");
                // Prendi instr from server, che indicherà la quantità di card presenti nella lavagna
                // Se il byte di stato è INSTR_EMPTY il secondo byte non è significativo, e la lavagna è
                // vuota
                // Comunico il mio intento al server
                instr_to_server[0] = INSTR_SHOW_LAVAGNA;
                send_msg(server_sock, instr_to_server, 2);
                fprintf(stderr, "[dbg]: mi faccio dire da lavagna quante card ho da ricevere\n");
                get_msg(server_sock, instr_from_server, 2);
                if(instr_from_server[0] == INSTR_EMPTY)
                {
                    libera_lavagna(lavagna);
                    lavagna = NULL;
                    show_lavagna(lavagna);
                    break;
                }
                uint8_t count = instr_from_server[1];
                for(uint8_t i = 0; i < count; i++)
                {
                    fprintf(stderr, "[dbg] main, valuto card %d-esima\n", i+1);
                    // TODO Error checking
                    // ricevo dimensione descrizione card
                    get_msg(server_sock, instr_from_server, 2);
                    // ricevo la card
                    task_card_t *cc = recive_card(server_sock, instr_from_server[1]);
                    // la inserisco nella lavagna vuota
                    insert_into_lavagna(&lavagna, cc);
                    free(cc);
                }
                show_lavagna(lavagna);
                libera_lavagna(lavagna);
                lavagna = NULL;
                break;
        }
    }
    return 0;
}
