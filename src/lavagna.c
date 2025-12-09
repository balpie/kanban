#include "../include/common.h"
#include "../include/lavagna.h"
#include "../include/lavagna_net.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h> 
#include <pthread.h>

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
                show_lavagna(lavagna);
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
    connection_l_e* conn = insert_connection(&(lista_connessioni.head), socket, ntohs(port), addr); 
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


    // TODO Se sono presenti card todo
    // e ho più di 2 utenti, mando la card a tutti così quelli 
    // scelgono chi la fa... 
    // Per fare questo da lato client a questo punto aspetto un messaggio dal server
    // che indichi cosa fare 
    // casi possibili sono:
    //  - card da fare disponibili
    //      quindi ricevi card, lista connessi e contatta i connessi
    //  - card da fare non disponibili
    //      quindi aspetta, eventualmente manda card
    //  - ping (se l'utente ha una card in todo)
    //      questo 
      
    // il primo byte  indica lo stato (se ci sono card disponibili, o altro)
    // il secondo byte indica eventuale dimensione prossimo messaggio (mai più di 255)
    // non è significativo nel caso in cui non sia implicita un'altra trasmissione
    // nello stato indicato dal primo byte
// argomento passato: connection_l relativo al client da servire

void* serv_client(void* cl_info) 
{
  // socket   
    struct client_info *cl_in = (struct client_info*)cl_info;
    // Sotto variabile che punta alla connessione servita da questo thread
    connection_l_e* connessione = registra_client(cl_in->socket, cl_in->addr); 
    free(cl_in);
    if(!connessione) // caso più di un utente con la stessa porta
    {
        return NULL;
    }
    unsigned char instr_to_client[2]; // messaggio di istruzioni da mandare al client
    unsigned char instr_from_client[2]; // messaggio di istruzioni da ricevere dal client
    for(;;)
    {
        instr_to_client[0] = INSTR_NOP; // nulla da fare
                                // quindi non mi interessa cosa c'è in instr_to_client[2]
        send_msg(connessione->socket, instr_to_client, 2);
            // passo al client la possibilità di decidere che fare  
        if(!get_msg(connessione->socket, instr_from_client, 2)) 
        { // caso connessione chiusa
            pthread_mutex_lock(&lista_connessioni.m);
            remove_connection(&(lista_connessioni.head), connessione->socket);
            pthread_mutex_unlock(&lista_connessioni.m);
            break;
        }
        switch(instr_from_client[0]) // in base a cosa vuole fare il client lo servo...
        {
        // RECIVE_CARD
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
            insert_into_lavagna(&lavagna, card); // salva la descrizione nella lista
            free(card); 

            show_lavagna(lavagna);
            printf("\nlavagna> ");
            fflush(stdout);
            break;
        case INSTR_SHOW_LAVAGNA:
            printf(">> Invia tutte le card della lavagna all'utente\n");
            send_lavagna(connessione->socket, lavagna);
            break;
        case INSTR_NOP:
            // Ne client ne server hanno "da fare"
            sleep(1); 
            break;
        }
    } 
    return NULL;
}

void stampa_utenti_connessi(connection_l_e *head)
{
    int count = 0;
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
