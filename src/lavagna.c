#include "../include/common.h"
#include "../include/lavagna.h"
#include "../include/lavagna_net.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h> 
#include <pthread.h>
#include <string.h>

// delay attesa per la ricezione del pong.
// Una volta terminato questo delay il relativo utente verrà disconnesso, 
// e la card tornerà in colonna TO-DO
struct timeval timeout_pong = {
    .tv_sec = TIME_PONG_MAX_DELAY,
    .tv_usec = 0
};      

// Ciclo del prompt
void* prompt_cycle(void *arg)
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
    printf("<< ripulisco socket e heap...\n");  
    cleanup(sock_listener, &lista_connessioni.head);
    exit(0);
}

// Ritorna la card con identificatore id.
// Se non presente ritorna NULL
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

// Ritorna la connessione dell'utente identificato da 
// port. Se non presente ritorna NULL
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

// Effettua la registrazione del client, se ha una porta valida, 
// non ancora presa da altri client
connection_l_e* registra_client(int socket, uint32_t addr) 
{
    char instr_to_client[2];
    uint16_t port; // port_id
    // il primo messaggio che ogni client deve mandare è la propria porta
    // identificativa, che gli permette di parlare con gli altri
    
    if(!get_msg(socket, (void*)&port, 2))
    {
        // non ho ancora creato la connessione quindi 
        close(socket);
        return NULL;
    }
    pthread_mutex_lock(&status.m);
    pthread_mutex_lock(&lista_connessioni.m);
    // se c'è già un utente con tale id gli comunico che l'iscrizione non è andata a buon fine
    if(get_connection(ntohs(port))) 
    {
        pthread_mutex_unlock(&lista_connessioni.m);
        pthread_mutex_unlock(&status.m);
        instr_to_client[0] = instr_to_client[1] = INSTR_TAKEN; 
        send_msg(socket, instr_to_client, 2);
        close(socket);
        return NULL;
    }
    if(status.status == INSTR_AVAL_CARD)
    {
        pthread_mutex_unlock(&lista_connessioni.m);
        pthread_mutex_unlock(&status.m);
        instr_to_client[0] = instr_to_client[1] = INSTR_WAIT; 
        send_msg(socket, instr_to_client, 2);
        close(socket);
        return NULL;
    }
    // Comunico al client che l'iscrizione è andata a buon fine
    instr_to_client[0] = instr_to_client[1] = INSTR_ACK; 
    send_msg(socket, instr_to_client, 2);
    connection_l_e* conn = insert_connection(&(lista_connessioni.head), socket, ntohs(port), ntohl(addr)); 
    pthread_mutex_unlock(&lista_connessioni.m);
    status.n_connessioni++;
    LOG("listener: numero connessioni %d\n", status.n_connessioni + 1);
    pthread_mutex_unlock(&status.m);
    return conn; 
}

// Manda via sock tutte le card della lavagna 
void send_lavagna(int sock, lavagna_t *lavagna)
{
    LOG("send_lavagna\n");
    // conto numero di card
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
    LOG("send_lavagna, \n\tcount: %d\n", instr_to_client[1]);
    // comunico al client la quantità di card da ricevere
    send_msg(sock, instr_to_client, 2); 
    p = lavagna;
    for(uint8_t i = 0; i < count; i++)
    {
        if(!send_card(sock, &p->card))
        {
            ERR("show_lavagna: send_card fallita\n");
        }
        p = p->next;
    }
}

// prepara status e la lista di connessioni a una send 
// fa il contrario di restore status
// Necessita di lock su status, connessione, e lavagna
void prepare_send()
{
    int count = 0;
    connection_l_e *p = lista_connessioni.head;
    while(p)
    {
        p->to_send = &lavagna->card;
        p = p->next;
        count++;
    }
    // assegno a total il numero di connessioni relative all'asta
    status.total = count;
    // assegno a sent 0 (numero di client pronti a fare l'asta)
    status.sent = 0;
    // setto lo status a INSTR_AVAL_CARD, e successivamente
    // comunico ai client che devono fare l'asta
    status.status = INSTR_AVAL_CARD;
}

// Viene passato aquired, ovvero il tempo trascorso
// dall'ultima volta che l'utente ha acquisito una card in doing
// La funzione valuta l'attuale stato del server, 
// e in base ad esso sceglie che operazione dovrà essere mandata
// al client. 
// Possibili operazioni: ping, carta disponibile, nessuna_instruzione
uint8_t chose_instr(time_t aquired, connection_l_e *connessione)
{
    if(aquired && (time(NULL) - aquired > TIME_PING))
    {
        // Se è passato più di TIME_PING il thread deve mandare il ping alla
        // propria connessione
        // questo valore non viene assegnato alla variabile status in quanto
        // è valido solo per il thread chiamante
        return INSTR_PING; 
    }
    // Ordine locking: per evitare deadlock: status->connessioni->lavagna
    // Ordine rilascio locking: lavagna->connessioni->status
    pthread_mutex_lock(&status.m);
    pthread_mutex_lock(&lista_connessioni.m);
    pthread_rwlock_rdlock(&m_lavagna);
    // altrimenti si valuta se è presente una card assegnabile
    if(lavagna && lavagna->card.colonna == TODO_COL)
    {
        // ... e se ci sono almeno due connessioni attive
        if(status.n_connessioni >= 2)
        {
            if(!connessione->to_send)
            {
                prepare_send();
            }
            // alle connessioni presenti auttualmente metto come
            // tosend la prima card di todo_list
            pthread_rwlock_unlock(&m_lavagna);
            pthread_mutex_unlock(&lista_connessioni.m);
            pthread_mutex_unlock(&status.m);
            return INSTR_AVAL_CARD;
        }
    }
    pthread_rwlock_unlock(&m_lavagna);
    pthread_mutex_unlock(&lista_connessioni.m);
    pthread_mutex_unlock(&status.m);
    // altrimenti lo status rimane invariato
    return status.status; 
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
    if(!prec) 
    {// inserimento in testa
        elem->next = iter;
        *lp = elem;
        return;
    }
    prec->next = elem;
    elem->next = iter;
}

// disconnette l'utente della connessione, gestendo sue eventuali card 
// mostra a schermo la lavagna se è cambiata
void disconnect_user(connection_l_e* connessione)
{
    int changed = 0;
    uint16_t port_id = connessione->port_id;
    pthread_mutex_lock(&lista_connessioni.m);
    remove_connection(&(lista_connessioni.head), connessione->socket);
    pthread_mutex_unlock(&lista_connessioni.m);
    pthread_mutex_lock(&status.m);
    status.n_connessioni--;
    pthread_mutex_unlock(&status.m);
    pthread_rwlock_wrlock(&m_lavagna);
    lavagna_t *pl = lavagna;
    // scorro la lavagna, sposto le card in doing (dell'utente disconnesso)
    // e le metto in todo, mentre tolgo l'assegnamento per le card in done e todo
    while(pl)
    {
        if(pl->card.utente == port_id)
        {
            switch(pl->card.colonna)
            {

                case DOING_COL:
                    changed = 1;
                    lavagna_t *cc = extract_from_lavagna(&lavagna, (uint8_t)pl->card.id);
                    if(cc) 
                    {
                        cc->card.last_modified = time(NULL);
                        cc->card.colonna = TODO_COL;
                        cc->card.utente = NO_USR;
                        insert_lavagna_elem(&lavagna, cc);
                    }
                    break;
                case TODO_COL:
                case DONE_COL:
                    changed = 1;
                    pl->card.utente = NO_USR;
            }
        }
        pl = pl->next;
    }
    // Se la lavagna ha subito modifiche la mostro a schermo
    if(changed)
    {
        show_lavagna(lavagna);
        printf("\nlavagna> ");
        fflush(stdout);
    }
    pthread_rwlock_unlock(&m_lavagna);
    // connessione terminata, termino thread
    pthread_exit(NULL); 
}

// funzione che manda al client le informazioni necessarie alla comunicazione p2p
void send_p2p_info(connection_l_e *connessione)
{
    unsigned char instr_to_client[2]; // messaggio di istruzioni da mandare al client
    unsigned char instr_from_client[2]; // messaggio di istruzioni da ricevere dal client

    instr_to_client[0] = INSTR_AVAL_CARD;
    instr_to_client[1] = status.total - 1;
    // dico al client che una card è avaliable, e gli dico il numero di peer che avrà
    send_msg(connessione->socket, instr_to_client, 2); 
    // mando la lista di tutti tranne se 
    LOG("send_p2p_info(%u): mando %u connessioni\n", connessione->port_id, instr_to_client[1]);
    send_conn_list(connessione->socket, connessione, status.total - 1);
    // mando la card, che è la prima di lavagna
    DBG("send_p2p_info(%u): mando la card %u\n", connessione->port_id, lavagna->card.id);
    send_card(connessione->socket, &lavagna->card);
    // aspetto su conditional variable di stato che tutti i thread abbiano mandato 
    // card e connessioni al proprio utente
    LOG("send_p2p_info(%u): ricevo ack\n", connessione->port_id);
    int msglen = get_msg(connessione->socket, instr_from_client, 2);
    // Caso connessione chiusa
    if(!msglen) 
    {
        disconnect_user(connessione);
    }
    // Il client ha tardato a rispondere (non dovrebbe succedere) 
    if(msglen < 0)
    {
        ERR("%u) Messaggio di ack peers non arrivato\n", connessione->port_id);
    }
    // Assumo sia un ack perchè non mi può mandare altro

    pthread_mutex_lock(&status.m);
    status.sent++;
    LOG("send_p2p_info(%u): status.total %u\tstatus.sent %u\n", connessione->port_id ,status.total, status.sent);
    // Condition variable per sincronizzare tutti i client
    while(status.sent < status.total)
    {
        pthread_cond_wait(&status.cv, &status.m);
    }
    // sveglio gli altri...
    pthread_cond_broadcast(&status.cv);

    // dico all'utente che può mandare le cose agli altri, visto che
    // hanno ricevuto dati su altri client e card
    instr_to_client[0] = instr_to_client[1] = INSTR_CLIENTS_READY;
    send_msg(connessione->socket, instr_to_client, 2);
    pthread_mutex_unlock(&status.m);
}

// Assegna card dell'asta alla connessione con id winner
int assign_card(uint16_t winner)
{
    if(!VALID_PORT(winner))
    {
        return 0;
    }
    pthread_rwlock_wrlock(&m_lavagna);
    LOG("assign_card: preso lock lavagna, setto effettivamente winner: %d\n", winner);
    lavagna_t* contended = extract_from_lavagna(&lavagna, lavagna->card.id);
    contended->card.colonna = DOING_COL;
    contended->card.utente = winner;
    contended->card.last_modified = time(NULL);

    insert_lavagna_elem(&lavagna, contended);
    // mostro la lavagna a video visto che l'ho cambiata
    show_lavagna(lavagna); 
    pthread_rwlock_unlock(&m_lavagna);
    printf("\nlavagna> ");
    fflush(stdout);
    return 1;
}

// Funzione chiamata una volta terminata un asta. Ripristina 
// status e connessioni in modo che si possa effettuare un'altra asta
void restore_status()
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

uint16_t handle_p2p_result(connection_l_e *connessione, uint16_t winner)
{
    pthread_mutex_lock(&status.m);
    status.winner_arrived++;
    // Barriera per fine asta. 
    // Senza si rischia di avere aste inconsistenti in caso
    // di più card disponibili in to-do
    TST("recv_p2p_result(%u): arrivo alla barrierera\n", connessione->port_id);
    if(status.winner_arrived == status.total)
    {
        // Finchè non è finita l'asta, aspetto sulla condition variable fine asta
        if(!assign_card(winner))
        {
            ERR("Utente %d ha passato un vincitore inesistente\n", connessione->port_id);
        }
        // A questo punto devo disfare le cose di status
        // ordine lock: status, connessioni, lavagna
        DBG("recv_p2p_result(%u): chiamata restore_status\n", connessione->port_id);
        restore_status();
        pthread_cond_broadcast(&status.fa); 
    }
    else
    {
        while(status.winner_arrived > 0)
        {
            TST("recv_p2p_result(%u): dentro la barrierera, arrived: %d, total: %d\n", 
                    connessione->port_id, status.winner_arrived, status.total);
            pthread_cond_wait(&status.fa, &status.m);
        }
    }   
    // risveglio tutti
    pthread_mutex_unlock(&status.m);
    return winner;
}

// Ricevi dall'utente il risultato dell'asta. Se sei l'ultimo, disfai
// status e inserisci nella lavagna. Ritorna il vincitore dell'asta
uint16_t recv_p2p_result(connection_l_e* connessione)
{
    // messaggio di istruzioni da ricevere dal client
    unsigned char instr_from_client[2]; 
    // caso disconnessione utente durante l'asta
    if(!get_msg(connessione->socket, instr_from_client, 2)) 
    {
        pthread_mutex_lock(&status.m);
        status.total--;
        pthread_mutex_unlock(&status.m);
        // La funzione termina il thread
        disconnect_user(connessione); 
    }
    uint16_t winner;
    // Il prossimo messaggio è il vincitore
    memcpy((void*)&winner, instr_from_client, 2);
    winner = ntohs(winner); 
    LOG("recv_p2p_result: La task va a: %u\n", winner);
    return handle_p2p_result(connessione, winner);
}

// ritorna l'id della card in doing dell'utente port, -1 se non presente
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

// funzionoe del thread principale per la gestione della comunicazione c-s
void* serv_client(void* cl_info) 
{
    // socket   
    struct client_info *cl_in = (struct client_info*)cl_info;
    // variabile che punta alla connessione servita da questo thread
    connection_l_e* connessione = registra_client(cl_in->socket, cl_in->addr); 
    free(cl_in);
    if(!connessione) // caso più di un utente con la stessa porta o utente disconnesso
    {
        pthread_exit(NULL);
    }
    // messaggio di istruzioni da mandare al client
    unsigned char instr_to_client[2]; 
    // messaggio di istruzioni da ricevere dal client
    unsigned char instr_from_client[2]; 
    // variabile che indica se ho già mandato la card e le info in caso di ciclo p2p
    int sent = 0; 
    // timestamp in cui la mia connessione ha acquisito l'ultima card che ha in doing
    time_t aquired = 0;

    for(;;)
    {
        instr_to_client[0] = chose_instr(aquired, connessione); 
        if(instr_to_client[0] == INSTR_AVAL_CARD)
        {
            if(!sent)
            {
                sent = 1;
                // mando al client le informazioni che gli servono per l'asta
                LOG("serv_client(%u) parte funzione send_p2p_info\n", connessione->port_id);
                send_p2p_info(connessione);
                // A questo punto aspetto da ogni client la risposta, che dovrebbe essere 
                // il numero di porta del client che si "aggiudica" la task
                // se sono l'ultimo, inserisco la card del vincitore nella lavagna
                // e poi "ripulisco" status 
                LOG("serv_client(%u) parte funzione recv_p2p_result\n", connessione->port_id);
                // ricevo risultati p2p, se la "il mio utente ha ricevuto la card allora faccio partire il conto"
                if(recv_p2p_result(connessione) == connessione->port_id && !aquired)
                {
                    LOG("serv_client(%u) adesso misuro timestamp aquired\n", connessione->port_id);
                    aquired = time(NULL); // Faccio partire il timer per ping-pong, se non è ancora partito
                }
                // Aspetta su condition variable e poi rimetti sent a 0
                LOG("serv_client(%u) rimetto sent a 0\n", connessione->port_id);
                sent = 0;
                continue;
            }
            else 
            {
                LOG("serv_client(%u): caso in cui sono già stati mandati i dati all'utente, "
                        "ed è stata ricevuta una risposta\n", connessione->port_id);
                usleep(500000); 
                continue;
            }
        }
        if(instr_to_client[0] == INSTR_PING)
        {
            send_msg(connessione->socket, instr_to_client, 2);
            LOG("serv_client(%u): mando ping\n"
                    "\tmi aspetto risposta entro %u\n"
                    ,connessione->port_id, TIME_PONG_MAX_DELAY);
            setsockopt(connessione->socket, SOL_SOCKET, SO_RCVTIMEO, &timeout_pong, sizeof(timeout_pong));
            int msglen = get_msg(connessione->socket, instr_from_client, 2);
            if(msglen <= 0) // Se l'utente si è esplicitament disconnesso, 
                            // oppure se il timeout è scaduto
            {
                disconnect_user(connessione); 
            }
            // timetto il timeout per il normale funzionamento
            setsockopt(connessione->socket, SOL_SOCKET, SO_RCVTIMEO, &timeout_recv, sizeof(timeout_recv));
            // riparto a contare il tempo di aquired visto che sono qui
            aquired = time(NULL); 
            continue;
        }
        if(instr_to_client[0] != INSTR_NOP)
            send_msg(connessione->socket, instr_to_client, 2);
        // passo al client la possibilità di decidere che fare  
        int msg_width = get_msg(connessione->socket, instr_from_client, 2);
        if(!msg_width) 
        { // caso connessione chiusa
            disconnect_user(connessione);
        }
        // caso INSTR_NOP arrivato dal client
        if(msg_width < 0) 
        {
            continue;
        }
        // in base a cosa vuole fare il client lo servo...
        switch(instr_from_client[0]) 
        {
        case INSTR_NEW_CARD:
            task_card_t *card = recive_card(connessione->socket, (size_t)instr_from_client[1]);
            // Nel caso in cui ci sia già una card presente con quell'id
            if(get_card(card->id))
            {
                // Comunico al client l'inserimento non riuscito
                instr_to_client[0] = instr_to_client[1] = INSTR_TAKEN;
                send_msg(connessione->socket, instr_to_client, 2);
                LOG("serv_client id card ricevuta già presente\n");
                free(card);
                break;
            }
            // Comunico al client l'inserimento riuscito
            instr_to_client[0] = instr_to_client[1] = INSTR_ACK;
            send_msg(connessione->socket, instr_to_client, 2);

            pthread_rwlock_wrlock(&m_lavagna);
            // salva la descrizione nella lista
            insert_into_lavagna(&lavagna, card); 
            pthread_rwlock_unlock(&m_lavagna);
            free(card); 

            pthread_rwlock_rdlock(&m_lavagna);
            show_lavagna(lavagna);
            pthread_rwlock_unlock(&m_lavagna);
            printf("\nlavagna> ");
            fflush(stdout);
            break;
        case INSTR_SHOW_LAVAGNA:
            pthread_rwlock_rdlock(&m_lavagna);
            send_lavagna(connessione->socket, lavagna);
            pthread_rwlock_unlock(&m_lavagna);
            break;
        case INSTR_NOP:
            break;
        case INSTR_CARD_DONE:
            pthread_rwlock_wrlock(&m_lavagna);
            LOG("se esiste, metto la card dell'utente in done\n");
            lavagna_t* done_card = extract_from_lavagna(&lavagna, instr_from_client[1]);
            DBG("sposto carta %d da doing a done: \n", done_card->card.id);
            if(done_card)
            {
                LOG("La card esisteva\n");
                done_card->card.colonna = DONE_COL;
                done_card->card.last_modified = time(NULL);
                insert_lavagna_elem(&lavagna, done_card);
                if(get_doing_card_id(connessione->port_id) == -1) // se l'utente non ha nessuna card in doing
                {
                    aquired = 0; // azzero il suo timer per ping-pong
                }
                show_lavagna(lavagna); // mostro la lavagna a video
                printf("\nlavagna> ");
                fflush(stdout);
            }
            else
            {
                LOG("serv_client(%u) ha provato a fare card_done su carta non presente nella lavagna\n", connessione->port_id);
            }
            pthread_rwlock_unlock(&m_lavagna);
        }
    } 
    return NULL;
}

// Stampa utenti connessi e informazioni riguardo lo stato attuale della lavagna
void stampa_utenti_connessi(connection_l_e *head)
{
    int count = 0;
    printf("<< status attuale: ");
    // Lo stato può soltanto essere nulla da fare o carta disponibile
    switch(status.status)
    {
        case INSTR_NOP:
            printf("niente da fare\n");
            break;
        case INSTR_AVAL_CARD:
            printf("card disponibile per asta\n");
            break;
    }
    printf("<< ci sono %d utenti connessi in totale: \n", status.n_connessioni);
    while(head!= NULL)
    {
        printf("%d<< port %u\n", ++count, head->port_id);
        head = head->next;
    }
}

// chiude socket e pulisce memoria
void cleanup(int listener_sock, connection_l_e** list)
{
    close(listener_sock);
    while(*list)
    {
        remove_connection(list, (*list)->socket);
    }
}

char* get_desc(FILE *cfile)
{
    // Conto quanto è grande la descrizione
    long inizio = ftell(cfile);
    int c;
    do
    {
        c = getc(cfile);
    }while(c != EOF && c != '\n');
    long fine = ftell(cfile);
    unsigned len = fine - inizio;
    if(len <= 1)
    {
        ERR("descrizione vuota\n");
        return NULL;
    }
    LOG("dimensione stringa: %u\n", len);
    if(fseek(cfile, inizio, SEEK_SET))
    {
        ERR("get_desc: errore fseek\n");
        return NULL;
    }
    char* desc = (char*)malloc(len);
    int offs = 0;
    do
    {
        desc[offs++] = getc(cfile);
    }while(desc[offs-1] != '\n' && desc[offs-1] != EOF);
    desc[len-1] = '\0';
    return desc;
}

task_card_t *parse_card(FILE *cfile)
{
    task_card_t *cc = (task_card_t *)malloc(sizeof(task_card_t));
    int correct = fscanf(cfile, "%hhu\n%hhu\n%hu\n%lu\n", 
            &cc->id, &cc->colonna, &cc->utente, &cc->last_modified);
    if(correct == EOF)
    {
        free(cc);
        LOG("Raggiunta la fine del file %s\n", INITIAL_CARDS);
        return NULL;
    }
    if(correct != 4)
    {
        ERR("Formato di %s errato, abort\n", INITIAL_CARDS);
        exit(-1);
    }
    cc->desc = get_desc(cfile);
    return cc;
}

int init_lavagna(){
    task_card_t* card;
    FILE *cards_file = fopen(INITIAL_CARDS, "r");
    if(!cards_file)
    {
        return 0;
    }
    while((card = parse_card(cards_file))){
        insert_into_lavagna(&lavagna, card);
        free(card);
    }
    return 1;
}

