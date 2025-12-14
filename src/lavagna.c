#include "../include/common.h"
#include "../include/lavagna.h"
#include "../include/lavagna_net.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h> 
#include <pthread.h>
#include <string.h>

void* prompt_cycle(void *)
{
    char cmd;
    do{
        cmd = prompt_line("lavagna");
        switch(cmd)
        {
            case CMD_NOP:
                break;
            case CMD_STAMPA_UTENTI_CONNESSI:
                pthread_mutex_lock(&lista_connessioni.m);
                stampa_utenti_connessi(lista_connessioni.head);
                pthread_mutex_unlock(&lista_connessioni.m);
                break;
            case CMD_SHOW_LAVAGNA:
                pthread_rwlock_rdlock(&m_lavagna);
                show_lavagna(lavagna);
                pthread_rwlock_unlock(&m_lavagna);
                break;
            case CMD_INVALID:
            default:
                printf(">! comando inesistente, o prefisso comune a più comandi\n");
                break;
        }
    }while(cmd != CMD_QUIT);
    printf(">> ripulisco socket e heap...\n");  
    cleanup(sock_listener, &lista_connessioni.head);
    exit(0);
}

lavagna_t* get_card(uint8_t id)
{
    lavagna_t* p = lavagna;
    while(p)
    {
        if(p->card.id == id)
        {
            return p;
        }
        p = p->next;
    }
    return NULL;
}

connection_l_e* get_connection(uint16_t port)
{
    connection_l_e* p = lista_connessioni.head;
    while(p)
    {
        if(p->port_id == port)
        {
            return p;
        }
        p = p->next;
    }
    return NULL;
}

connection_l_e* registra_client(int socket, uint32_t addr) // TODO error checking
{
    char instr_to_client[2];
    uint16_t port; // port_id
    // il primo messaggio che ogni client deve mandare è la propria porta
    // identificativa, che gli permette di parlare con gli altri
    get_msg(socket, (void*)&port, 2); 
    pthread_mutex_lock(&lista_connessioni.m);
    if(get_connection(ntohs(port))) // se c'è già un utente con tale id
    {
        pthread_mutex_unlock(&lista_connessioni.m);
        instr_to_client[0] = instr_to_client[1] = INSTR_TAKEN; // Comunico al client che la registrazione
                                                               // non è andata a buon fine
        send_msg(socket, instr_to_client, 2);
        close(socket);
        return NULL;
    }
    instr_to_client[0] = instr_to_client[1] = INSTR_ACK; // Comunico al client che la registrazione è 
                                                         // andata a buon fine
    send_msg(socket, instr_to_client, 2);
    connection_l_e* conn = insert_connection(&(lista_connessioni.head), socket, ntohs(port), ntohl(addr)); 
    pthread_mutex_unlock(&lista_connessioni.m);
    status.n_connessioni++;
    return conn; // assumo successo
}

void send_lavagna(int sock ,lavagna_t *lavagna)
{
    fprintf(stderr, "[dbg] send_lavagna\n");
    // conto numero di cards
    lavagna_t *p = lavagna;
    uint8_t count = 0;
    while(p)
    {
        count++;
        p = p->next;
    }
    unsigned char instr_to_client[2]; 
    if(!count)
    {
        instr_to_client[0] = instr_to_client[1] = INSTR_EMPTY;
        send_msg(sock, instr_to_client, 2);
        return;
    }
    instr_to_client[0] = INSTR_SHOW_LAVAGNA;
    instr_to_client[1] = count;
    fprintf(stderr, "[dbg] send_lavagna, \n\tcount: %d\n", instr_to_client[1]);
    send_msg(sock, instr_to_client, 2); // invio la quantità di card
    p = lavagna;
    for(uint8_t i = 0; i < count; i++)
    {
        send_card(sock, &p->card);
        p = p->next;
    }
}

uint8_t eval_status(time_t aquired)
{
    // Ordine locking: prima status, poi connessioni, poi lavagna
    // Se lo stato è già questo, c'è già una avaliable card che sta essendo processata
    //fprintf(stderr, "[dbg] eval_status\n\taquired %lu\n\telapsed %lu\n\t", aquired, time(NULL) - aquired);
    if(aquired && (time(NULL) - aquired > TIME_PING))
    {
        return INSTR_PING;
    }
    pthread_mutex_lock(&status.m);
    if(status.status == INSTR_AVAL_CARD)
    {
        //fprintf(stderr, "[dbg] eval_status: status era INSTR_AVAL_CARD e rimane così\n");
        pthread_mutex_unlock(&status.m);
        return INSTR_AVAL_CARD;
    }
    pthread_rwlock_rdlock(&m_lavagna);
    // altrimenti si valuta se è presente una card assegnabile
    if(lavagna && lavagna->card.colonna == TODO_COL)
    {
        //fprintf(stderr, "[dbg] eval_status: card presente in colonna todo\n");
        if(status.n_connessioni >= 2)
        {
            //fprintf(stderr, "[dbg] eval_status: status.n_connessioni > 2 => imposto status per comunicazione p2p\n");
            // alle connessioni presenti auttualmente metto come
            // tosend la prima card di todo_list
            int count = 0;
            connection_l_e *p = lista_connessioni.head;
            while(p)
            {
                p->to_send = &lavagna->card;
                p = p->next;
                count++;
            }
            status.total = count;
            status.sent = 0;
            status.status = INSTR_AVAL_CARD;
            pthread_mutex_unlock(&status.m);
            pthread_rwlock_unlock(&m_lavagna);
            return INSTR_AVAL_CARD;
        }
    }
    pthread_mutex_unlock(&status.m);
    pthread_rwlock_unlock(&m_lavagna);
    //fprintf(stderr, "[dbg] eval_status: status attuale %u\n", status.status);
    return status.status; // lo status rimane il solito (che sia users_choosing o instr_nop)
}

// come insert_into_lavagna ma inserisce direttamente un elemento e non ne alloca uno nuovo
void insert_lavagna_elem(lavagna_t** lp, lavagna_t*elem)
{
    if(!*lp) 
    {
        (*lp) = elem;
        elem->next = NULL;
        return;
    }
    lavagna_t *iter = *lp;
    lavagna_t *prec = NULL;
    while(iter && iter->card.colonna < elem->card.colonna)
    {
        prec = iter;
        iter = iter->next;
    }
    if(!prec) // inserimento in testa
    {
        elem->next = iter;
        *lp = elem;
        return;
    }
    prec->next = elem;
    elem->next = iter;
}

// funzione che manda al client le informazioni necessarie alla comunicazione p2p
void send_p2p_info(connection_l_e *connessione)
 {
    unsigned char instr_to_client[2]; // messaggio di istruzioni da mandare al client
    unsigned char instr_from_client[2]; // messaggio di istruzioni da ricevere dal client

    instr_to_client[0] = INSTR_AVAL_CARD;
    // TODO concorrenza
    instr_to_client[1] = status.total - 1;
    // dico al client che una card è avaliable, e gli dico il numero di peer che avrà
    fprintf(stderr, "[dbg] send_p2p_info: mando al client INSTR_AVAL_CARD | %u peers\n", instr_to_client[1]);
    send_msg(connessione->socket, instr_to_client, 2); 
    // mando la lista di tutti tranne se 
    fprintf(stderr, "[dbg] send_p2p_info: mando %u connessioni\n", instr_to_client[1]);
    send_conn_list(connessione->socket, connessione, status.total - 1);
    // mando la card, che è la prima di lavagna
    fprintf(stderr, "[dbg] send_p2p_info: mando la card\n");
    send_card(connessione->socket, &lavagna->card);
    // aspetto su conditional variable di stato che tutti i thread abbiano mandato 
    // card e connessioni al proprio utente
    fprintf(stderr, "[dbg] send_p2p_info: ricevo ack\n");
    get_msg(connessione->socket, instr_from_client, 2); 
    // assumo sia un ack perchè non mi può mandare altro
    // Ora che so che il mio client ha ricevuto card e cose 
    status.sent++;

    pthread_mutex_lock(&status.m);
    fprintf(stderr, "[dbg] send_p2p_info: total %u\tsent %u\n", status.total, status.sent);
    while(status.total != status.sent)
    {
        pthread_cond_wait(&status.cv, &status.m);
    }
    // sveglio gli altri...
    pthread_cond_broadcast(&status.cv);
    instr_to_client[0] = instr_to_client[1] = INSTR_CLIENTS_READY;
    // Qui dico all'utente che può mandare le cose agli altri, visto che
    // hanno ricevuto dati su altri client e card
    send_msg(connessione->socket, instr_to_client, 2);
    pthread_mutex_unlock(&status.m);
}

// Ricevi dall'utente il risultato dell'asta. Se sei l'ultimo, disfai
// status e inserisci nella lavagna. Ritorna il vincitore dell'asta
uint16_t recv_p2p_result(connection_l_e* connessione)
{
    unsigned char instr_from_client[2]; // messaggio di istruzioni da ricevere dal client
    get_msg(connessione->socket, instr_from_client, 2);
    uint16_t winner;
    memcpy((void*)&winner, instr_from_client, 2);
    winner = ntohs(winner); 
    fprintf(stderr, "[dbg] serv_client: La task va a: %u\n", winner);

    pthread_mutex_lock(&status.m);
    if(++status.winner_arrived == status.total)
    {
        pthread_rwlock_wrlock(&m_lavagna);
        fprintf(stderr, "[dbg] serv_client: preso lock lavagna, setto effettivamente winner: %d\n", winner);
        lavagna_t* contended = extract_from_lavagna(&lavagna, lavagna->card.id);
        contended->card.colonna = DOING_COL;
        contended->card.utente = winner;
        insert_lavagna_elem(&lavagna, contended);
        pthread_rwlock_unlock(&m_lavagna);
        
        // A questo punto devo disfare le cose di status
        // ordine lock: status, connessioni, lavagna
        if(status.total != 0)
        {
            status.winner_arrived = 0;
            status.total = 0;
            status.sent = 0;
            pthread_mutex_lock(&lista_connessioni.m);
            connection_l_e *p = lista_connessioni.head;
            while(p)
            {
                if(p->to_send)
                    p->to_send = NULL;
                p = p->next;
            }
            pthread_mutex_unlock(&lista_connessioni.m);
            status.status = INSTR_NOP;
        }
    }
    pthread_mutex_unlock(&status.m);
    return winner;
}

// ritorna l'id della card in doing dell'utente port, -1 se non presente'
int get_doing_card_id(uint16_t port)
{
    lavagna_t *p = lavagna;
    while(p)
    {
        if(p->card.colonna == DOING_COL && p->card.utente == port)
            return p->card.id;
        if(p->card.colonna == DONE_COL)
            break;
        p = p->next;
    }
    return -1;
}

void* serv_client(void* cl_info) 
{
    // socket   
    struct client_info *cl_in = (struct client_info*)cl_info;
    // variabile che punta alla connessione servita da questo thread
    connection_l_e* connessione = registra_client(cl_in->socket, cl_in->addr); 
    free(cl_in);
    if(!connessione) // caso più di un utente con la stessa porta
    {
        return NULL;
    }
    unsigned char instr_to_client[2]; // messaggio di istruzioni da mandare al client
    unsigned char instr_from_client[2]; // messaggio di istruzioni da ricevere dal client
    int sent = 0; // variabile che indica se ho già mandato la card e le info in caso di ciclo p2p
    time_t aquired = 0;

    unsigned char old_instr_to_client = INSTR_NOP;

    for(;;)
    {
        // istante in cui l'utente ha acquisito la card
        instr_to_client[0] = eval_status(aquired); 
        if(old_instr_to_client != instr_to_client[0])
        {
            if(old_instr_to_client == INSTR_AVAL_CARD)
            {
                // se è finito il ciclo di p2p rimetto sent a 0 per il prossimo
                sent = 0;    
            }
            fprintf(stderr, "[dbg] serv_client(%u): cambio status, da %u a %u\n",
                    connessione->port_id, old_instr_to_client, instr_to_client[0]);
            old_instr_to_client = instr_to_client[0];
        }
        if(instr_to_client[0] == INSTR_AVAL_CARD)
        {
            if(!sent)
            {
                sent = 1;
                // mando al client le informazioni che gli servono per l'asta
                fprintf(stderr, "[dbg]serv_client(%u) parte funzione send_p2p_info\n", connessione->port_id);
                send_p2p_info(connessione);
                // A questo punto aspetto da ogni client la risposta, che dovrebbe essere 
                // il numero di porta del client che si "aggiudica" la task
                // se sono l'ultimo, inserisco la card del vincitore nella lavagna
                // e poi "ripulisco" status 
                fprintf(stderr, "[dbg]serv_client(%u) parte funzione recv_p2p_result\n", connessione->port_id);
                // ricevo risultati p2p, se la "il mio utente ha ricevuto la card allora faccio partire il conto"
                if(recv_p2p_result(connessione) == connessione->port_id && !aquired)
                {
                    aquired = time(NULL); // Faccio partire il timer per ping-pong, se non è ancora partito
                }
                fprintf(stderr, "[dbg]serv_client(%u) rimetto sent a 0\n", connessione->port_id);
                continue;
            }
            else // se il thread ha già mandato, aspetta
            {
                sleep(1); 
                continue;
            }
        }
        if(instr_to_client[0] == INSTR_PING)
        {
            send_msg(connessione->socket, instr_to_client, 2);
            time_t ping_sent = time(NULL);
            fprintf(stderr, "[dbg]serv_client(%u): mando ping\n", connessione->port_id);
            get_msg(connessione->socket, instr_from_client, 2);
            time_t elapsed = time(NULL) - ping_sent; // tempo impiegato dall'utente a rispondermi
            fprintf(stderr, "[dbg]serv_client(%u): tempo impiegato dall client a rispondermi al ping %lu\n",
                    connessione->port_id, elapsed);
            if(elapsed > TIME_PONG_MAX_DELAY)
            { 
                // disconnetto il client, tolgo carte in doing, e le metto in todo
                fprintf(stderr, "[dbg]serv_client(%u) : %u non ha mandato il pong in tempo",
                        connessione->port_id, connessione->port_id);
                int card_id;
                // finchè ci sono card in doing per l'utente...
                pthread_rwlock_wrlock(&m_lavagna);
                // TODO fai rimozione card in doing in rimuovi connessione
                while((card_id = get_doing_card_id(connessione->port_id)) != -1)
                {
                    fprintf(stderr, "[dbg]serv_client(%u) : sposto card %d in todo\n",
                            connessione->port_id, card_id);
                    lavagna_t *cc = extract_from_lavagna(&lavagna, (uint8_t)card_id);
                    if(cc) // non dovrebbe servire 
                    {
                        cc->card.colonna = TODO_COL;
                        cc->card.utente = NO_USR;
                        insert_lavagna_elem(&lavagna, cc);
                    }
                }
                pthread_rwlock_unlock(&m_lavagna);

                pthread_mutex_lock(&lista_connessioni.m);
                remove_connection(&(lista_connessioni.head), connessione->socket);
                pthread_mutex_unlock(&lista_connessioni.m);

                pthread_mutex_lock(&status.m);
                status.n_connessioni--;
                pthread_mutex_unlock(&status.m);

                pthread_exit(NULL); // connessione terminata, termino thread
            }
            // riparto a contare il tempo di aquired visto che sono qui
            aquired = time(NULL); 
            continue;
        }
        send_msg(connessione->socket, instr_to_client, 2);
            // passo al client la possibilità di decidere che fare  
        if(!get_msg(connessione->socket, instr_from_client, 2)) 
        { // caso connessione chiusa
            pthread_mutex_lock(&lista_connessioni.m);
            remove_connection(&(lista_connessioni.head), connessione->socket);
            pthread_mutex_unlock(&lista_connessioni.m);
            pthread_mutex_lock(&status.m);
            status.n_connessioni--;
            pthread_mutex_unlock(&status.m);
            pthread_exit(NULL); // connessione terminata, termino thread
        }
        switch(instr_from_client[0]) // in base a cosa vuole fare il client lo servo...
        {
        case INSTR_NEW_CARD:
            task_card_t *card = recive_card(connessione->socket, (size_t)instr_from_client[1]);
            if(get_card(card->id))
            {
                // Comunico al client l'inserimento non riuscito
                instr_to_client[0] = instr_to_client[1] = INSTR_TAKEN;
                send_msg(connessione->socket, instr_to_client, 2);
                fprintf(stderr, "[dbg] serv_client id card ricevuta già presente\n");
                free(card);
                break;
            }
            // Comunico al client l'inserimento riuscito
            instr_to_client[0] = instr_to_client[1] = INSTR_ACK;
            send_msg(connessione->socket, instr_to_client, 2);

            pthread_rwlock_wrlock(&m_lavagna);
            insert_into_lavagna(&lavagna, card); // salva la descrizione nella lista
            pthread_rwlock_unlock(&m_lavagna);

            free(card); 

            pthread_rwlock_rdlock(&m_lavagna);
            show_lavagna(lavagna);
            pthread_rwlock_unlock(&m_lavagna);
            printf("\nlavagna> ");
            fflush(stdout);
            break;
        case INSTR_SHOW_LAVAGNA:
            printf("\n>> Invia tutte le card della lavagna all'utente\n");
            pthread_rwlock_rdlock(&m_lavagna);
            send_lavagna(connessione->socket, lavagna);
            pthread_rwlock_unlock(&m_lavagna);
            break;
        case INSTR_NOP:
            // Ne client ne server hanno "da fare"
            sleep(1);  // TODO setsockopt SO_RECVTIMEOUT
            break;
        case INSTR_CARD_DONE:
            // TODO Finisci
            pthread_rwlock_wrlock(&m_lavagna);
            fprintf(stderr, "[dbg] se esiste, metto la card dell'utente in done");
            lavagna_t* done_card = extract_from_lavagna(&lavagna, lavagna->card.id);
            if(done_card)
            {
                fprintf(stderr, "[dbg] La card esisteva");
                done_card->card.colonna = DONE_COL;
                insert_lavagna_elem(&lavagna, done_card);
                if(get_doing_card_id(connessione->port_id) == -1) // se l'utente non ha nessuna card in doing
                {
                    aquired = 0; // azzero il suo timer per ping-pong
                }
            }
            pthread_rwlock_unlock(&m_lavagna);
        }
    } 
    return NULL;
}

void stampa_utenti_connessi(connection_l_e *head)
{
    int count = 0;
    printf(">> ci sono %d utenti connessi in totale: \n", status.n_connessioni);
    while(head!= NULL)
    {
        printf("%d>> port %u\n", ++count, head->port_id);
        head = head->next;
    }
}

void cleanup(int listener_sock, connection_l_e** list)
{
    close(listener_sock);
    while(*list)
    {
        remove_connection(list, (*list)->socket);
    }
}
