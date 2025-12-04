#include "../include/common.h"
#include "../include/common_net.h"
#include "../include/utente.h"
#include "../include/utente_net.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

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
        
    for(;;)
    {
        char c;
        if(cmd_tail == cmd_head)
        {
            //printf(">! stato della coda: \t\ntail: %d\t\nhead: %d\n", cmd_tail, cmd_head);
            //printf(">! nulla da fare per il client thread, aspetto npo\n");
            //fflush(stdout);
            sleep(1);
            continue;
        }
        c = cmd_queue[cmd_tail];
        cmd_tail = (cmd_tail + 1) % MAX_QUEUE_CMD;
        switch(c)
        {
            case CMD_CREATE_CARD:
                pthread_mutex_lock(&created_m);
                printf("\n>> mandando card appena creata...\n");
                send_card(server_sock, created); // manda card al server
                free(created->desc); // libero la descrizione, anc'essa allocata nello heap
                free(created); // libero la card
                pthread_mutex_unlock(&created_m);
                printf("\n>> carta mandata!\n");
                printf("utente> ");
                fflush(stdout);
                break;
        }
    }

    return 0;
}
