#include "../include/utente.h"
#include "../include/common.h"
#include "../include/common_net.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

char prompt_msg[12];

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
        close(listener);
        exit(-1);
    }
    int nport = htons(port);
    if(send_msg(sd, &nport, 2)) // mando al server la mia porta per registrarmi
    {
        char instr_from_server[2];
        int msglen = get_msg(sd, instr_from_server, 2);
        if(!msglen)
        {
            disconnect(sd);
        }
        if(instr_from_server[1] == INSTR_TAKEN)
        {
            printf(">! registrazione fallita: porta già utilizzata da un altro client\n");
            close(sd);
            close(listener);
            exit(-1);
        }
    }
    else
    {
        printf("<< registrazione fallita per motivo server\n");
        close(sd);
        close(listener);
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
        printf("<< inserire la descrizione dell'attività, da terminare con a-capo. Massimo %d caratteri:\n", 
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
    char* desc = (char*)malloc((strlen(buf) + 1)*sizeof(char));
    strcpy(desc, buf);
    return desc;
}

int get_col()
{
    char buf[2];
    int count = 0;
    do{
        if(count > 0)
        {
            printf(">! la colonna DEVE essere 0, 1 o 2\n");
        }
        printf("\t0: To Do\n");
        printf("\t1: Doing\n");
        printf("\t2: Done\n");
        printf("<< inserire colonna: ");
        if(!fgets(buf, 2, stdin)) // 1 cifra + \n
        {
            perror(">! errore fgets, la card non è stata creata");
            return -1;
        }
        buf[1] = '\0'; // Sicuramente ho un a-capo nel buffer
        clear_stdin_buffer();
        count++;
    }
    while(buf[0] < '0' || buf[0] > '2');
    return buf[0] - '0';
}

// Ritorna 1 in caso di successo, 0 altrimenti
task_card_t *create_card()
{
    task_card_t *new_card = (task_card_t*)malloc(sizeof(task_card_t));
    char buf[MAX_DIM_DESC];
    printf("<< inserire id (0 <= id < 256): ");
    if(!fgets(buf, 4, stdin)) // 3 cifre + \n
    {
        printf(">! errore fgets, la card non è stata creata");
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
    new_card->colonna = get_col();
    if(new_card->colonna < 0)
    {
        free(new_card);
        return NULL;
    }
    new_card->last_modified = time(NULL);
    new_card->desc = get_desc(buf);
    if(!new_card->desc)
    {
        free(new_card);
        return NULL;
    }
    new_card->utente = NO_USR; // ancora non è assegnata a nessun utente
    return new_card;
}

void disconnect(int server_sock)
{
    close(listener);
    close(server_sock);
    LOG("arrivato comando QUIT\n");
    _exit(0); // uso questa in modo da terminare anche il thread fermo su fgets
}

void *prompt_cycle_function(void* self_info)
{
    int* port_sock = (int*)self_info;
    //int user_port = port_sock[0];
    int server_sock = port_sock[1];
    char c;
    for(;;)
    {
        c = prompt_line(prompt_msg);
        if((cmd_head + 1) % MAX_QUEUE_CMD == cmd_tail)
        {
            // caso coda piena
            printf(">! coda comandi piena, i prossimi comandi verranno ignorati\n");
            continue;
        }
        switch(c)
        {
            case CMD_NOP:
                // no comando
                continue;
            case CMD_QUIT:
                // disconnessione e terminazione programma
                disconnect(server_sock);
            case CMD_INVALID:
            case CMD_STAMPA_UTENTI_CONNESSI:
                printf(">! comando inesistente, o prefisso comune a più comandi\n");
                continue;
            case CMD_CREATE_CARD:
                if(!pthread_mutex_trylock(&created_m))
                {
                    created = create_card();  // crea card
                    pthread_mutex_unlock(&created_m);
                }
                else
                {
                    printf(">! Impossibile creare la carta adesso\n");
                    continue;
                }
                break;
            case CMD_SHOW_LAVAGNA:
                LOG( "prompt: Arrivato comando show lavagna\n\tcmd_head: %d\n\tcmd_tail: %d\n",
                        cmd_head, cmd_tail);
                break;
        }   
        pthread_mutex_lock(&cmd_queue_m);
        cmd_queue[cmd_head] = c;
        cmd_head = (cmd_head + 1) % MAX_QUEUE_CMD;
        pthread_mutex_unlock(&cmd_queue_m);
    }
}

