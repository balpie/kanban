#include "../include/common.h"
#include "../include/common_net.h"
#include "../include/utente.h"
#include "../include/utente_net.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <time.h>

lavagna_t *lavagna = NULL;
// coda circolare dei comandi
// Visto che siamo in caso Single Producer - Single Consumer
// la coda circolare dovrebbe essere thread safe.
// TODO NON E VERO PERCHE IL COMPILATORE PUO RIORDINARE 
char cmd_queue[MAX_QUEUE_CMD];
task_card_t *created = NULL;
lavagna_t *doing = NULL;
pthread_mutex_t created_m;
int cmd_head = 0;
int cmd_tail = 0;
int listener;

void err_args(char* prg)
{
        printf(">! utilizzo: %s <porta> [-d]\n", prg);
        printf(">! il parametro <porta> deve essere un intero rappresentabile su 16 bit maggiore di 5678\n");
        printf(">! passare argomento -d per redirigere il logging da file a schermo\n");
        exit(-1);
}

int main(int argc, char* argv[])
{
    srand(time(NULL));
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
    if(argc != 3 || strcmp(argv[2], "-d"))// se gli argomenti non sono esattamente 2, e il secondo non è -d
    {
        // TODO error check
        // redirigo i messaggi di errore a un file
        printf(">> Redirigo stderr a un file\n");

        // nome log file di dimensione strlen(prefisso) + (massimo numero cifre uint16_t) + '\0'
        char logfilename[strlen(COMMON_LOGFILE_NAME) + 5 + 1];
        sprintf(logfilename, "%s%u", COMMON_LOGFILE_NAME, user_port);
        int logfile = open(logfilename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        dup2(logfile, STDERR_FILENO); // associo stderr al logfile
        fprintf(stderr, "[dbg] sto per chidere fd: %d\n", logfile);
        close(logfile);
        fflush(stderr);
    }
    printf(">> Registrazione al server con porta %u...\n", user_port);
    int server_sock = registra_utente(user_port);
    int self_info[2] = {user_port, server_sock};

    struct sockaddr_in listener_addr;
    int listener = init_listener(&listener_addr, user_port); // listener socket per interazione p2p


    pthread_create(&prompt_cycle, NULL, prompt_cycle_function, self_info);
    pthread_detach(prompt_cycle);

    // variabile per logging
    unsigned char old_instr_from_server = 0;
        
    for(;;) // Ciclo di comunicazione con server
    {
        peer_list *peers = NULL;
        unsigned char instr_to_server[2];
        unsigned char instr_from_server[2];

        get_msg(server_sock, instr_from_server, 2);
        // DEBUG
        if(instr_from_server[0] != old_instr_from_server)
        {
            fprintf(stderr, "[dbg] main: Ricevuto da server %u\n", instr_from_server[0]);
            old_instr_from_server = instr_from_server[0];
        }
        if(instr_from_server[0] == INSTR_AVAL_CARD)
        {
            // Lista di peer per questa card
            fprintf(stderr, "[dbg] main: devono arrivare %u peers\n", instr_from_server[1]);
            for(uint8_t i = 0; i < instr_from_server[1]; i++)
            {
                peer_list *pp = recive_peer(server_sock);
                if(pp)
                {
                    fprintf(stderr, "[dbg] chiamo insert_peer su \n\taddr: %u\n\tport: %u\n", 
                            pp->addr, pp->port);
                    insert_peer(&peers, pp);
                }
                else
                {
                    fprintf(stderr, "[dbg] main: errore ricezione peer\n");
                }
            }
            // aggiungo la porta di questo processo questo semplifica l'"iterazione" p2p
            peer_list *pp = (peer_list*)malloc(sizeof(peer_list));
            pp->port = user_port;
            pp->addr = 0; 
            pp->sock = -1;
            pp->next = NULL;
            insert_peer(&peers, pp);

            // richiedo la dimensione della card
            get_msg(server_sock, instr_from_server, 2);
            fprintf(stderr, "[dbg] main, card arriva di dimensione %u\n", instr_from_server[1]);
            task_card_t *contended_card = recive_card(server_sock, (uint8_t)instr_from_server[1]);

            fprintf(stderr, "[dbg] main, card ricevuta: \n");
            show_card(contended_card);

            // Mando ack per dire al server che ho ricevuto card e peers
            instr_to_server[0] = instr_to_server[1] = INSTR_ACK_PEERS; 
            send_msg(server_sock, instr_to_server, 2);

            // Aspetto che tutti i client siano pronti
            get_msg(server_sock, instr_from_server, 2);

            // p2p()...
            // funzione di utilità per fine di testing e debug

            print_peers(peers);
            uint16_t winner = NO_USR;
            peer_list *p = peers;
            while(kanban_p2p_iteration(listener, peers, p, user_port, &winner))
            {
                p = p->next;
            }
            fprintf(stderr, "[dbg] mando porta vincitore, tale: %u\n", winner);
            uint16_t network_winner = htons(winner);

            fprintf(stderr, "[dbg] user_port %u\twinner %u\n", user_port ,winner);
            if(winner == user_port)
            {
                printf(">> Ho proposto il costo minore, quindi mi prendo la card\n");
                insert_into_lavagna(&doing, contended_card);
            }
            free(contended_card);
            send_msg(server_sock, (void*)&network_winner, 2);
            // ho finito, dealloco la lista dei peer
            deallocate_list(&peers);
            continue;
        }
        if(instr_from_server[0] == INSTR_PING)
        { // se mi arriva ping mando pong // TODO test
            instr_to_server[0] = INSTR_PING;
            send_msg(server_sock, instr_to_server, 2);
            continue;
        }
        char c;
        if(cmd_tail == cmd_head) // caso in cui non ho comandi da eseguire
        {
            instr_to_server[0] = INSTR_NOP;
            send_msg(server_sock, instr_to_server, 2); // comunico al server che anche il client
                                                       // non ha nulla da fare
            sleep(1); // TODO setsockopt SO_RECVTIMEOUT
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

                 // libero la card 
                free(created); 
                pthread_mutex_unlock(&created_m);
                printf("\n>> carta mandata!\n");
                get_msg(server_sock, instr_from_server, 2);
                if(instr_from_server[0] == INSTR_ACK)
                {
                    printf("\n>> carta valida!\n");
                }
                else
                {
                    printf("\n>> carta non valida!\n");
                }
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
            case CMD_CARD_DONE:
                // Per ora l'utente può finire le task con ordine LIFO
                if(!doing)
                {
                    printf(">! nessuna carta in doing, comando equivalente a NOP\n");
                    break;
                }
                instr_to_server[0] = INSTR_CARD_DONE;
                instr_to_server[1] = doing->card.id; 
                fprintf(stderr, "[dbg] Estraggo dalla lavagna\n");
                extract_from_lavagna(&doing, doing->card.id); // estrazione in testa
                send_msg(server_sock, instr_to_server, 2);
                break;
        }
    }
    close(listener);
    return 0;
}

