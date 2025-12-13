#include "../include/utente_net.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

// TODO: rendi timed le cose in modo che se si disconnette un peer non blocca tutti.

// implementazione parte p2p utente
peer_list *recive_peer(int sock)
{
    unsigned char buf[6];
    get_msg(sock, buf, 6);
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
    fprintf(stderr, "[dbg] partita insert_peer, *pl = %p\n", *pl);
    while(*pl && (*pl)->port > elem->port)
    {
        fprintf(stderr, "[dbg] insert_peer, scorro dopo peer con porta: %u\n", (*pl)->port);
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
        if(tmp->sock != -1)
        {
            close(tmp->sock);
        }
        free(tmp);
    }
}

void print_peers(peer_list* list)
{
    int cont = 0;
    while(list)
    {
        cont++;
        fprintf(stderr, "[dbg]: -%d- addr %u\tport %u\n", cont, list->addr, list->port);
        list = list->next;
    }
}

// genera un costo casuale da un minimo di 1 a un massimo di 255
uint8_t generate_cost()
{
    return rand() % 256;
}

int send_cost(peer_list* lst, uint8_t cost)
{
    fprintf(stderr, "[dbg] send cost: mando costo(%u) a tutti\n", cost);
    int aperto = 1; // indica se il socket della connessione che sto valutando è già aperto 
    int count = 0; // conta il numero di peer ai quali è stato mandato il costo
    while(lst)
    {
        if(lst->addr == 0) // L'elemento nella lista rappresenta me
        {
            fprintf(stderr, "[dbg] send_cost: trovato me stesso nella lista\n");
            aperto = 0;
            lst = lst->next;
            continue;
        }
        if(!aperto) // non è ancora stato aperto il socket per questa connessione
        {
            fprintf(stderr, "[dbg] send_cost: apro socket per %u\n", lst->port);
            lst->sock = socket(AF_INET, SOCK_STREAM, 0);
            struct sockaddr_in addr_peer;
            memset(&addr_peer, 0, sizeof(addr_peer));
            addr_peer.sin_port = htons(lst->port);
            fprintf(stderr, "[dbg] send_cost: provo a fare la connect con %u\n\taddr %u\n",
                    ntohs(addr_peer.sin_port), lst->addr);
            addr_peer.sin_addr.s_addr = htonl(lst->addr);
            addr_peer.sin_family = AF_INET;
            // DEBUG
            char ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &addr_peer.sin_addr, ip, sizeof(ip));
            fprintf(stderr,
            "[dbg] connect -> %s:%u (raw addr=%u)\n",
            ip, lst->port, lst->addr);
            if(connect(lst->sock, (struct sockaddr*)&addr_peer, sizeof(addr_peer)))
            {
                fprintf(stderr, "[err] connect fallita\n\tport %u\taddr %u\n", lst->port, lst->addr);
                perror("[err] send_cost: errore connect");
                lst = lst->next;
                continue;
            }
        }
        fprintf(stderr, "[dbg] faccio la send\n");
        if(send(lst->sock, (void*)&cost, 1, 0) != 1)
        {
            fprintf(stderr, "[err] errore send\n\tport %u\taddr %u\n", lst->port, lst->addr);
            lst = lst->next;
            continue;
        }
        count++;
        lst = lst->next;
    }
    return count;
}

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
        fprintf(stderr, "[dbg] kanban_p2p: è il mio turno di mandare\n");
        curr_cost = generate_cost();
        // sent contiene il numero di processi ai quali è stato mandato il costo
        // quindi il numero di peer (senza contare il sender, processo attuale)
        int sent = send_cost(list, curr_cost);
        fprintf(stderr, "[dbg] mandato il costo a %u processi\n", sent);
        if(curr_cost <= curr_min_cost)
        {
            curr_min_cost = curr_cost;
            *winner_proc = next->port;
        }
        return 1; // continua
    }

    fprintf(stderr, "[dbg] kanban_p2p: mi aspetto una connect da parte di %u\n", next->port);
    struct sockaddr_in nuovo;
    unsigned int len_nuovo = sizeof(nuovo);

    // la accept è necessaria solo in caso di porta maggiore
    if(next->port > user_port)
    {
        fprintf(stderr, "[dbg] faccio accept per connessione con %u\n", next->port);
        next->sock = accept(sock, (struct sockaddr*)&nuovo, &len_nuovo);
    }
    if(next->sock < 1)
    {
        fprintf(stderr, 
                "[err] kanban_p2p_iteration, errore interno oppure errore accept:\n\tsocket connessione con %u non aperto\n",
                 next->port);
    }
    fprintf(stderr, "[dbg] faccio la recv\n");
    recv(next->sock, (void*)&curr_cost, 1, 0);
    fprintf(stderr, "[dbg] kanban_p2p: ricevuto costo da %u: %u\n", next->port, curr_cost);
    if(curr_cost < curr_min_cost)
    {
        curr_min_cost = curr_cost;
        *winner_proc = next->port;
    }
    return 1;
}

