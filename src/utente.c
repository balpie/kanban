#include "../include/utente.h"
#include "../include/common.h"
#include "../include/common_net.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Stringa contenente il prompt del client, così composto: 
// "utentexxxxx" dove le x rappresentano le 4 o 5 cifre della porta identificativa.
char prompt_msg[12];

// Comunica alla lavagna che una determinata card è stata terminata.
// Questa carta è quella in testa nella lista doing, quindi le card vengono 
// terminate con ordine lifo (come una stack)
int card_done(int server_sock, lavagna_t **doing_list)
{
    uint8_t instr_to_server[2];
    if(!*doing_list)
    {
        return 0; 
    }
    instr_to_server[0] = INSTR_CARD_DONE;
    instr_to_server[1] = doing->card.id; 
    LOG("Estraggo dalla lavagna\n");
    // estrazione in testa
    lavagna_t *cc = extract_from_lavagna(doing_list, 
            (*doing_list)->card.id); 
    DBG("card_done: estratto card %u\n", cc->card.id);
    // libero la card
    free(cc->card.desc); 
    free(cc);
    send_msg(server_sock, instr_to_server, 2);
    return 1;
}

void* worker_fun(void* no_arg__)
{
    // Segnalo che la carta in doing sta essendo processata
    worker_occupato = true;
    // Processazione
    sleep(MAX_TIME_DOING);
    // Segnalo che la carta in doing è stata
    worker_occupato = false;
    pthread_exit(NULL);
}


void insert_lavagna_coda(lavagna_t **l, const task_card_t *card)
{
    lavagna_t* new = (lavagna_t*)malloc(sizeof(lavagna_t)); 
    new->next = NULL;
    copia_card(card, &(new->card)); 
    if(!*l)
    {
        (*l) = new;
        return;
    }
    lavagna_t *iter = *l;
    while(iter->next)
    {
        iter = iter->next;
    }
    iter->next = new;
}

// valuta se la worker ha finito di eseguire la card
// Se deve essere mandata la manda, e ritorna 1.
int send_if_done(int server_sock) 
{
    // Se ci sono card in doing e worker non sta lavorando
    if(doing && !worker_occupato)
    {
        pthread_join(worker, NULL);
        card_done(server_sock, &doing); 
        // Se c'è un'altra carta da fare aggiorno il timestamp
        if(doing)
        {
            pthread_create(&worker, NULL, worker_fun, NULL);
        }
        TST("card fatta\n");
        return 1;
    }
    return 0;
}

// funzione di utilità: da chiamare in caso di output troppo lungo quando chiamata fgets
// toglie i caratteri in eccesso, fino ad arrivare al \n
void clear_stdin_buffer()
{
    char c;
    while((c = getchar()) != '\n'); 
}

// tenta di effettuare la connessione e registrazione del client,
// in caso di fallimento termina il processo
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
        perror(">! errore connect ");
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
        if(instr_from_server[1] == INSTR_WAIT)
        {
            printf(">! registrazione fallita: asta in corso\n");
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

// Riceve da stdin la descrizione di una card.
// non è accettata registrazione vuota.
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

// Riceve da stdin colonna della card.
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

// Ritorna la card in caso di successo, 0 altrimenti.
// Crea la card.
// E' richiesto il numero di porta solo per card già fatte, in quanto
// altrimenti si bypasserebbe il sistema dell'asta
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
    if(new_card->colonna != DONE_COL)
    {
        // ancora non è assegnata a nessun utente
        new_card->utente = NO_USR; 
    }
    else
    {
        // 5 cifre base 10 per la porta, 1 per \n
        printf("<< inserire porta dell'utente (5679 <= id < 65536): ");
        fgets(buf, 6, stdin);
        new_card->utente = strtoul(buf, NULL, 10); 
        if(new_card->utente == 0 || !VALID_PORT(new_card->utente))
        {
            new_card->utente = NO_USR;
        }
        char* endlptr = strchr(buf, '\n');
        if(!endlptr)
        { 
            clear_stdin_buffer();
        }
    }

    new_card->last_modified = time(NULL);
    new_card->desc = get_desc(buf);
    if(!new_card->desc)
    {
        free(new_card);
        return NULL;
    }
    return new_card;
}

void disconnect(int server_sock)
{
    close(listener);
    close(server_sock);
    LOG("arrivato comando QUIT\n");
    // devo usare questa funzione per terminare tutto il processo, 
    // incluso un eventuale thread fermo su fgets (per esempio in create_card)
    _exit(0); 
}

// Funzione per il thread relativo all'interazione con il thread via terminale
void *prompt_cycle_function(void* self_info)
{
    int* port_sock = (int*)self_info;
    int server_sock = port_sock[1];
    char c;
    for(;;)
    {
        c = prompt_line(prompt_msg);
        pthread_mutex_lock(&cmd_queue_m);
        if((cmd_head + 1) % MAX_QUEUE_CMD == cmd_tail)
        {
            // caso coda piena
            pthread_mutex_unlock(&cmd_queue_m);
            printf(">! coda comandi piena, i prossimi comandi verranno ignorati\n");
            continue;
        }
        pthread_mutex_unlock(&cmd_queue_m);
        switch(c)
        {
            case CMD_NOP:
                // non fare nulla
                continue;
            case CMD_QUIT:
                // disconnessione e terminazione programma
                disconnect(server_sock);
                // cmd invalid va con stampa utenti connessi (comando 
                // di lavagna, non di utente)
            case CMD_INVALID:
            case CMD_STAMPA_UTENTI_CONNESSI:
                printf(">! comando inesistente, o prefisso comune a più comandi\n");
                continue;
            case CMD_CREATE_CARD:
                // Se c'è già una carta creata, e sta essendo processata, 
                // non deve essere possibile che l'utente provi a crearne un'altra
                if(!pthread_mutex_trylock(&created_m))
                {
                    // In caso contrario questa viene creata
                    created = create_card();  
                    printf("\n%s> ", user_prompt);
                    fflush(stdout);
                    pthread_mutex_unlock(&created_m);
                }
                else
                {
                    printf(">! Impossibile creare la carta adesso\n");
                    continue;
                }
                break;
            case CMD_SHOW_LAVAGNA:
                LOG("prompt: Arrivato comando show lavagna\n\tcmd_head: %d\n\tcmd_tail: %d\n",
                        cmd_head, cmd_tail);
                break;
        }   
        // Se arrivato qui, c'è un comando valido e il thread di 
        // comunicazione con la lavagna ha ciò che serve perchè possa essere eseguito:
        // Posso inserire il comando nella cmdqueue
        pthread_mutex_lock(&cmd_queue_m);
        cmd_queue[cmd_head] = c; 
        cmd_head = (cmd_head + 1) % MAX_QUEUE_CMD;
        pthread_mutex_unlock(&cmd_queue_m);
    }
}
