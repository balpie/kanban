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
            case CMD_INVALID:
                printf(">> comando inesistente\n");
                break;
            case CMD_STAMPA_UTENTI_CONNESSI:
                pthread_mutex_lock(&lista_connessioni.m);
                stampa_utenti_connessi(lista_connessioni.head);
                pthread_mutex_unlock(&lista_connessioni.m);
                break;
        }
    }while(cmd != CMD_QUIT);
    printf(">> ripulisco socket e heap...\n");  
    cleanup(sock_listener, &lista_connessioni.head);
    exit(0);
}

  
// argomento passato: connection_l relativo al client da servire
void* serv_client(void* cl_info) 
{
    struct client_info *cl_in = (struct client_info*)cl_info;
    int socket = cl_in->socket;
    uint32_t addr = cl_in->addr;
    free(cl_in);
    uint16_t port; // port_id
    // il primo messaggio che ogni client deve mandare è la propria porta
    // identificativa, che gli permette di parlare con gli altri
    get_msg(socket, (void*)&port, 2); 
    pthread_mutex_lock(&lista_connessioni.m);
    insert_connection(&(lista_connessioni.head), socket, ntohs(port), addr); 
    pthread_mutex_unlock(&lista_connessioni.m);
    n_connessioni++;
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
    unsigned char instr_to_client[2]; 
    // come sopra ma per info su messaggi ricevuti dal client
    unsigned char instr_from_client[2];
    for(;;)
    { // TODO error handling
        instr_to_client[0] = STS_NOCARDS; // nulla da fare
                                // quindi non mi interessa cosa c'è in instr_to_client[2]
        send_msg(socket, instr_to_client, 2);
            // passo al client la possibilità di decidere che fare  
        get_msg(socket, instr_from_client, 2); 
            // ora sicuramente mi dirà che mi vuole mandare una bellissima card
        size_t dim_desc_card = instr_from_client[1];
        char net_card[dim_desc_card]; // preparo il buffer per la card in versione network
        get_msg(socket, net_card, dim_desc_card + sizeof(task_card_t) - sizeof(char*));
        task_card_t card;
        unprepare_card(&card, net_card, dim_desc_card); // alloca la descrizione
        printf("dbg[serv_client]> card arrivata con colonna: %u\n", card.colonna);
        insert_into_lavagna(&lavagna, &card); // salva la descrizione nella lista
        // stampo di nuovo il prompt 
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
