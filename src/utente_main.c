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

char cmd_queue[MAX_QUEUE_CMD];
int cmd_head = 0;
int cmd_tail = 0;
pthread_mutex_t cmd_queue_m;
char user_prompt[13]; // utentexxxxxx\0

task_card_t *created = NULL;
pthread_mutex_t created_m; 

lavagna_t *doing = NULL; // non serve mutex: 
                         // struttura dati nota esclusivamente qui
time_t doing_timestamp = 0;  // nessuna card presa in carico

int listener;

// timeout socket con lavagna
struct timeval timeout_recv = {
    .tv_sec = 1,
    .tv_usec = 0
};      
// 3 secondi massimi di attesa in ingresso e in uscita ai socket
struct timeval timeout_p2p = {
    .tv_sec = 3,
    .tv_usec = 0
};


void err_args(char* prg)
{
        printf(">! utilizzo: %s <porta> [-d]\n", prg);
        printf(">! il parametro <porta> deve essere un intero rappresentabile su 16 bit maggiore di 5678\n");
        printf(">! passare argomento -d per redirigere il logging da file a schermo\n");
        _exit(-1);
}

void get_peers(peer_list **peers, int server_sock, unsigned char instr_from_server[2], uint16_t user_port)
{
    // Inizio get peer
    LOG("main: devono arrivare %u peers\n", instr_from_server[1]);
    for(uint8_t i = 0; i < instr_from_server[1]; i++)
    {
        peer_list *pp = recive_peer(server_sock);
        if(pp)
        {
            LOG("chiamo insert_peer su \n\taddr: %u\n\tport: %u\n", 
                    pp->addr, pp->port);
            insert_peer(peers, pp);
        }
        else
        {
            ERR("get_peers: peer non ricevuto correttamente o connessione chiusa\n");
            disconnect(server_sock);
        }
    }
    // aggiungo la porta di questo processo questo semplifica l'"iterazione" p2p
    peer_list *pp = (peer_list*)malloc(sizeof(peer_list));
    pp->port = user_port;
    pp->addr = 0; 
    pp->sock = -1;
    pp->next = NULL;
    insert_peer(peers, pp);
    // fine get peer
}

// Se presente, preleva comando dalla coda circolare, lo manda alla lavagna, e gestisce la risposta
void send_command(int server_sock)
{
    unsigned char instr_from_server[2];
    unsigned char instr_to_server[2];
    char c;
    pthread_mutex_lock(&cmd_queue_m);
    if(cmd_tail == cmd_head) // caso in cui non ho comandi da eseguire
    {
        pthread_mutex_unlock(&cmd_queue_m);
        // Torno ad aspettare che il server abbia qualcosa da fare
        return; 
    }
    c = cmd_queue[cmd_tail];
    cmd_tail = (cmd_tail + 1) % MAX_QUEUE_CMD;
    pthread_mutex_unlock(&cmd_queue_m);
    LOG("main: comando arrivato: %c\n", c);
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
            int msglen = get_msg(server_sock, instr_from_server, 2);
            if(!msglen)
            {
                disconnect(server_sock);
            }
            if(instr_from_server[0] == INSTR_ACK)
            {
                printf("\n>> carta valida!\n");
            }
            else
            {
                printf("\n>> carta non valida!\n");
            }
            printf("%s> ", user_prompt);
            fflush(stdout);
            break;
        case CMD_SHOW_LAVAGNA:
            LOG( "[dbg]: arrivato comando SHOW_LAVAGNA\n");
            // Prendi instr from server, che indicherà la quantità di card presenti nella lavagna
            // Se il byte di stato è INSTR_EMPTY il secondo byte non è significativo, e la lavagna è
            // vuota
            // Comunico il mio intento al server
            instr_to_server[0] = INSTR_SHOW_LAVAGNA;
            send_msg(server_sock, instr_to_server, 2);
            LOG("[dbg]: mi faccio dire da lavagna quante card ho da ricevere\n");
            msglen = get_msg(server_sock, instr_from_server, 2);
            if(!msglen)
            {
                disconnect(server_sock);
            }
            if(instr_from_server[0] == INSTR_EMPTY)
            {
                libera_lavagna(lavagna);
                lavagna = NULL;
                show_lavagna(lavagna);
                printf("\n%s> ", user_prompt);
                fflush(stdout);
                break;
            }
            uint8_t count = instr_from_server[1];
            for(uint8_t i = 0; i < count; i++)
            {
                LOG( "main, valuto card %d-esima\n", i+1);
                // ricevo dimensione descrizione card
                msglen = get_msg(server_sock, instr_from_server, 2);
                if(msglen < 0) LOG("Il messaggio non è arrivato!!!\n");
                if(!msglen)
                {
                    disconnect(server_sock);
                }
                // ricevo la card
                task_card_t *cc = recive_card(server_sock, instr_from_server[1]);
                // la inserisco nella lavagna vuota
                insert_into_lavagna(&lavagna, cc);
                free(cc);
            }
            show_lavagna(lavagna);
            printf("\n%s> ", user_prompt);
            fflush(stdout);
            libera_lavagna(lavagna);
            lavagna = NULL;
            break;
        case CMD_CARD_DONE:
            // Per ora l'utente può finire le task con ordine LIFO
            if(!card_done(server_sock, &doing))
            {
                printf("\n>! nessuna carta in doing\n");
                printf("%s> ", user_prompt);
                fflush(stdout);
            }
            break;
    }
}

int main(int argc, char* argv[])
{
    srand(time(NULL));
    memset(cmd_queue, 0, MAX_QUEUE_CMD);
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
    sprintf(user_prompt, "utente%u", user_port);
    pthread_mutex_init(&created_m, NULL);
    pthread_mutex_init(&cmd_queue_m, NULL);
    pthread_t prompt_cycle;
    sprintf(prompt_msg, "utente%u", user_port);
    if(argc != 3 || strcmp(argv[2], "-d"))// se gli argomenti non sono esattamente 2, e il secondo non è -d
    {
        // redirigo i messaggi di errore a un file
        printf(">> Redirigo stderr a un file\n");
        // nome log file di dimensione strlen(prefisso) + (massimo numero cifre uint16_t) + '\0'
        char logfilename[sizeof(COMMON_LOGFILE_NAME) + 6];
        sprintf(logfilename, "%s%u", COMMON_LOGFILE_NAME, user_port);
        int logfile = open(logfilename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if(logfile < 0)
        {
            printf(">! errore open logfile%s\n", logfilename);
        }
        if(dup2(logfile, STDERR_FILENO) < 0) // associo stderr al logfile
        {
            printf(">! errore ridirezione stderr\n");
        }
        LOG( "sto per chidere fd: %d\n", logfile);
        if(close(logfile))
        {
            printf(">! errore close\n");
        }
        fflush(stderr); 
    }
    printf("<< Registrazione al server con porta %u...\n", user_port);
    int server_sock = registra_utente(user_port);
    LOG( "do al socket timeout 1s in send\n");
    if (setsockopt (server_sock, SOL_SOCKET, SO_RCVTIMEO, &timeout_recv, sizeof(timeout_p2p)) < 0)
        ERR( "errore setsockopt\n");
    int self_info[2] = {user_port, server_sock};

    struct sockaddr_in listener_addr;
    int listener = init_listener(&listener_addr, user_port); // listener socket per interazione p2p
    if (setsockopt (listener, SOL_SOCKET, SO_RCVTIMEO, &timeout_p2p, sizeof(timeout_p2p)) < 0)
    {
        ERR( "errore setsockopt\n");
        printf("!< Errore: in caso di errori nell'asta e' "
                "possibile che si blocchi completamente l'applicazione\n");
    }

    pthread_create(&prompt_cycle, NULL, prompt_cycle_function, self_info);
    pthread_detach(prompt_cycle);

    // variabile per logging
    unsigned char old_instr_from_server = 0;
        
    for(;;) // Ciclo di comunicazione con server
    {
        peer_list *peers = NULL;
        unsigned char instr_to_server[2];
        unsigned char instr_from_server[2];

        int msglen = get_msg(server_sock, instr_from_server, 2);
        if(!msglen)
        { // caso lavagna disconnessa
            disconnect(server_sock);
        }
        if(msglen < 0) // Se il server non mi ha mandato nulla
        {
            // Se è passato abbasatanza tempo, mando la prima card in doing
            if(send_if_done(server_sock))
            {
                continue;
            }
            // mando, se c'è, un comando
            send_command(server_sock);
            continue;
        }

        LOG("main: msglen %d\n", msglen); 
        if(instr_from_server[0] != old_instr_from_server)
        {
            LOG("main: Ricevuto da server %u\n", instr_from_server[0]);
            old_instr_from_server = instr_from_server[0];
        }

        if(instr_from_server[0] == INSTR_AVAL_CARD)
        {
            // Lista di peer per questa card
            get_peers(&peers, server_sock, instr_from_server, user_port);

            // richiedo la dimensione della card
            msglen = get_msg(server_sock, instr_from_server, 2);
            if(msglen < 0) LOG("Il messaggio non è arrivato!!!\n");
            if(!msglen)
            {
                disconnect(server_sock);
            }
            LOG("main, card arriva di dimensione %u\n", instr_from_server[1]);
            task_card_t *contended_card = recive_card(server_sock, (uint8_t)instr_from_server[1]);

            LOG("main, card ricevuta\n");

            // Mando ack per dire al server che ho ricevuto card e peers
            instr_to_server[0] = instr_to_server[1] = INSTR_ACK_PEERS; 
            LOG("mando ack al server\n");
            send_msg(server_sock, instr_to_server, 2);

            // Aspetto che tutti i client siano pronti
            LOG( "Aspetto che tutti i client siano pronti\n");
            msglen = get_msg(server_sock, instr_from_server, 2);
            if(msglen < 0) LOG("Il messaggio non è arrivato!!!\n");
            if(!msglen)
            {
                disconnect(server_sock);
            }

            // p2p()...
            // funzione di utilità per fine di testing e debug

            print_peers(peers);
            uint16_t winner = NO_USR;
            peer_list *p = peers;
            while(kanban_p2p_iteration(listener, peers, p, user_port, &winner))
            {
                p = p->next;
            }
            LOG("mando porta vincitore, tale: %u\n", winner);
            uint16_t network_winner = htons(winner);

            LOG("user_port %u\twinner %u\n", user_port ,winner);
            if(winner == user_port) 
            {
                printf("\n>> Ho proposto il costo minore, quindi mi prendo la card\n%s> "
                        , user_prompt);
                fflush(stdout);
                insert_into_lavagna(&doing, contended_card);
                // Memorizzo il timestamp di arrivo della card
                // per poterla mandare passato un intervallo di tempo MAX_TIME_DOING secondi
                doing_timestamp = time(NULL);
            }
            free(contended_card);
            send_msg(server_sock, (void*)&network_winner, 2);
            // ho finito, dealloco la lista dei peer
            deallocate_list(&peers);
            continue;
        }
        if(instr_from_server[0] == INSTR_PING)
        { 
            instr_to_server[0] = INSTR_PONG;
            TST("Faccio arrivare apposta in ritardo il pong\n");
            sleep(20); 
            send_msg(server_sock, instr_to_server, 2);
            continue;
        }
    }
    close(listener);
    return 0;
}

