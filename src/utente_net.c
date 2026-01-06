#include "../include/utente_net.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

// Ricezione via socket di un singolo peer.
peer_list *recive_peer(int sock)
{
    // Metto un valore fantoccio in caso fallimento recv peer, 
    // in modo da vederlo nei log
    unsigned char buf[6];
    uint16_t *dummy_port = (uint16_t *) &buf[0];
    uint32_t *dummy_addr = (uint32_t *) &buf[2];
    *dummy_port = NO_USR;
    *dummy_addr = 0;
    
    int msglen;
    if((msglen = get_msg(sock, buf, 6)) < 0)
    {
        ERR("recive_peer: ricezione peer fallita\n");
        return NULL;
    }
    DBG("Arrivato peer, port: 0x%X, addr: 0x%X\n"
            , *dummy_port, *dummy_addr);
    if(!msglen) // Connessione chiusa
    {
        ERR("recive_peer: lavagna ha chiuso la connessione\n");
    }
    if(msglen < 0)
    {
        ERR("recive_peer ricezione time out\n");
    }
    peer_list* new = (peer_list*)malloc(sizeof(peer_list));
    memcpy(&new->port, &buf[0], 2);
    memcpy(&new->addr, &buf[2], 4);
    new->addr = ntohl(new->addr);
    new->port = ntohs(new->port);
    new->next = NULL;
    return new;
}

// Inserisce il peer nella lista, in modo da rendere la lista ordinata rispetto al
// numero di porta in modo decrescente
void insert_peer(peer_list** pl, peer_list* elem)
{
    LOG("insert_peer: partita insert_peer, *pl = %p\n", *pl);
    while(*pl && (*pl)->port > elem->port)
    {
        LOG("insert_peer: scorro dopo peer con porta: %u\n", (*pl)->port);
        pl = &((*pl)->next);
    }
    peer_list* tmp = *pl;
    *pl = elem;
    elem->next = tmp;
}

// Libera la memoria della peer_list
void deallocate_list(peer_list** pl)
{
    while(*pl)
    {
        peer_list* tmp = *pl;
        pl = &((*pl)->next);
        if(tmp->sock > 0)
        {
            LOG("deallocate_list: sto chiudendo fd %d\n", tmp->sock);
            close(tmp->sock);
        }
        free(tmp);
    }
}

// Utility: fa il log della lista di peer passata
void print_peers(peer_list* list)
{
    int cont = 0;
    while(list)
    {
        cont++;
        LOG("print_peers: -%d- addr 0x%X\tport %u\n", cont, list->addr, list->port);
        list = list->next;
    }
}

// genera un costo casuale 0 a 255
uint8_t generate_cost()
{
    return rand() % 256;
}

// Manda il proprio costo agli altri peer. Se necessario apre le
// rispettive connessioni TCP. Ritorna il numero di peer a cui si
// è riusciti a mandare il costo 
int send_cost(peer_list* lst, uint8_t cost)
{
    LOG("send cost: mando costo(%u) a tutti\n", cost);
    // indica se il socket della connessione che sto valutando è già aperto 
    int aperto = 1; 
    // conta il numero di peer ai quali è stato mandato il costo
    int count = 0; 
    while(lst)
    {
        // L'elemento della lista con indirizzo 0 divide 
        // i peer con cui la connessione è già aperta (perchè sono stati i primi a mandare
        // il proprio costo) e quelli con cui la connessione va aperta
        if(lst->addr == 0) 
        {
            LOG("send_cost: trovato me stesso nella lista\n");
            aperto = 0;
            lst = lst->next;
            continue;
        }
        // non è ancora stato aperto il socket per questa connessione
        if(!aperto) 
        {
            // Apro la connessione, e per poter costruire un meccanismo di fallback imposto un 
            // timeout sui socket, in modo che in caso di peer disconnesso il peer non rimanga in 
            // condizione di deadlock
            LOG("send_cost: apro socket per %u\n", lst->port);
            lst->sock = socket(AF_INET, SOCK_STREAM, 0);
            if(setsockopt (lst->sock, SOL_SOCKET, SO_RCVTIMEO, &timeout_p2p, sizeof(timeout_p2p)) < 0)
            {
                ERR("errore setsockopt\n");
            }
            if(setsockopt (lst->sock, SOL_SOCKET, SO_SNDTIMEO, &timeout_p2p, sizeof(timeout_p2p)) < 0)
            {
                ERR("errore setsockopt\n");
            }
            struct sockaddr_in addr_peer;
            memset(&addr_peer, 0, sizeof(addr_peer));
            addr_peer.sin_port = htons(lst->port);
            addr_peer.sin_addr.s_addr = htonl(lst->addr);
            addr_peer.sin_family = AF_INET;

            // Le seguienti 4 righe servono esclusivamente al logging e debug
            LOG("connect -> %u:%x\n", lst->port, lst->addr);

            if(connect(lst->sock, (struct sockaddr*)&addr_peer, sizeof(addr_peer)))
            {
                ERR("connect fallita\n\tport %u\taddr %u\n", lst->port, lst->addr);
                perror("[err] send_cost: errore connect");
                lst->sock = -1;
                lst = lst->next;
                continue;
            }
        }
        LOG("send a %u\n", lst->port);
        // Viene mandato il costo generato al peer valutato in quel momento
        if(send(lst->sock, (void*)&cost, 1, 0) != 1)
        {
            ERR("errore send\n\tport %u\taddr %u\n", lst->port, lst->addr);
            lst = lst->next;
            continue;
        }
        count++;
        lst = lst->next;
    }
    return count;
}

// Funzione da chiamare su ogni elemento della lista di peer.
// Va chiamata anche con NULL come lista per reimpostare 
// la variabile statica del costo minimo corrente.
// Se next ha come indirizzo 0 è il peer su cui questo processo 
// sta essendo eseguito, quindi si dovrà chiamare la send_cost.
// altrimenti bisognerà ricevere il costo dal relativo peer.
unsigned kanban_p2p_iteration(int sock, peer_list *list, peer_list* next, 
        uint16_t user_port,  uint16_t* winner_proc)
{
    // Variabile statica il cui valore rimane invariato tra un ciclo di iterazioni p2p e un altro
    static uint8_t curr_min_cost = 255;
    uint8_t curr_cost;
    if(!next)
    {
        // inizializzo le variabili per le prossime interazioni p2p 
        curr_min_cost = 255;
        // ritorno 0 per comunicare la fine delle comunicazioni p2p
        return 0;
    }
    if(user_port == next->port)
    {
        // Mando a tutti il mio costo
        LOG("kanban_p2p: è il mio turno di mandare\n");
        curr_cost = generate_cost();
        // sent contiene il numero di processi ai quali è stato mandato il costo
        // quindi il numero di peer (senza contare il sender, processo attuale)
        int sent = send_cost(list, curr_cost);
        LOG("kanban_p2p: mandato il costo a %u processi\n", sent);
        if(curr_cost <= curr_min_cost)
        {
            // Se il mio costo è migliore del minimo corrente, 
            // aggiorno il minimo corrente
            curr_min_cost = curr_cost;
            *winner_proc = next->port;
        }
        return 1; 
    }

    LOG("kanban_p2p: mi aspetto una connect da parte di %u\n", next->port);
    struct sockaddr_in nuovo;
    unsigned int len_nuovo = sizeof(nuovo);

    // la accept è necessaria solo in caso di porta maggiore
    if(next->port > user_port)
    {
        LOG("kanban_p2p: faccio accept per connessione con %u\n", next->port);
        next->sock = accept(sock, (struct sockaddr*)&nuovo, &len_nuovo);
    }
    if(next->sock < 1)
    {
        ERR("kanban_p2p_iteration, errore interno oppure errore accept:"
                "\n\tsocket connessione con %u non aperto\n",
                 next->port);
    }
    // metto timer 3s al nuovo socket, in modo che se un altro peer 
    // si disconnette non rimango in deadlock
    if(setsockopt(next->sock, SOL_SOCKET, SO_RCVTIMEO, &timeout_p2p, sizeof(timeout_p2p)) < 0)
    {
        ERR( "errore setsockopt\n");
    }
    if(setsockopt(next->sock, SOL_SOCKET, SO_SNDTIMEO, &timeout_p2p, sizeof(timeout_p2p)) < 0)
    {
        ERR( "errore setsockopt\n");
    }

    LOG("kanban_p2p: faccio la recv\n");
    ssize_t msglen = recv(next->sock, (void*)&curr_cost, 1, 0);
    if(msglen <= 0)
    {
        ERR("p2p: recv fallita\n");
        return 0;
    }
    LOG("kanban_p2p: ricevuto costo da %u: %u\n", next->port, curr_cost);
    if(curr_cost < curr_min_cost)
    {
        curr_min_cost = curr_cost;
        *winner_proc = next->port;
    }
    return 1;
}

