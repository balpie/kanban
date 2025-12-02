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
            case CMD_INVALID:
            default:
                printf(">! comando inesistente\n");
                break;
        }
    }while(cmd != CMD_QUIT);
    printf(">> ripulisco socket e heap...\n");  
    cleanup(sock_listener, &lista_connessioni.head);
    exit(0);
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
int registra_client(int socket, uint32_t addr) // TODO error checking
{
    uint16_t port; // port_id
    // il primo messaggio che ogni client deve mandare è la propria porta
    // identificativa, che gli permette di parlare con gli altri
    get_msg(socket, (void*)&port, 2); 
    pthread_mutex_lock(&lista_connessioni.m);
    insert_connection(&(lista_connessioni.head), socket, ntohs(port), addr); 
    pthread_mutex_unlock(&lista_connessioni.m);
    n_connessioni++;
    return 1; // assumo successo
}

void* serv_client(void* cl_info) 
{
    struct client_info *cl_in = (struct client_info*)cl_info;
    int socket = cl_in->socket;
    registra_client(socket, cl_in->addr); // TODO error checking
    free(cl_in);
    unsigned char instr_to_client[2]; 
    unsigned char instr_from_client[2];
    for(;;)
    { 
        instr_to_client[0] = STS_NOCARDS; // nulla da fare
                                // quindi non mi interessa cosa c'è in instr_to_client[2]
        send_msg(socket, instr_to_client, 2);
            // passo al client la possibilità di decidere che fare  
        get_msg(socket, instr_from_client, 2); 
        // RECIVE_CARD
        printf("dbg> recive_card, dim_desc_card: %lu\n", (size_t)instr_from_client[1]);
        task_card_t *card = recive_card(socket, (size_t)instr_from_client[1]);
        printf("dbg> insert_into_lavagna\n");
        insert_into_lavagna(&lavagna, card); // salva la descrizione nella lista
        free(card); 
                    
        printf("dbg> show_lavagna\n");
        show_lavagna(lavagna);
        printf("\nlavagna> ");
        fflush(stdout);
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
