#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "../include/common_net.h"

// Usando il preprocessore funziona sia per macchine che
// adottano little endian che per macchine che adottano 
// big endian, ammesso che usino gcc
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
uint64_t htonll(uint64_t num)
{
    return num;
}
uint64_t ntohll(uint64_t num)
{
    return num;
}
#else
uint64_t htonll(uint64_t num)
{
    /*
     *   least_sign    most_sign
     * | B0 B1 B2 B3 | B4 B5 B6 B7 |
     *
     * hton(least_sign) hton(most_sign)
     * | B3 B2 B1 B0 | B7 B6 B5 B4 |
     */
    uint32_t most_sign = htonl((num >> 32));// hton ultimi 32 bit
    uint32_t least_sign = htonl(num);// hton primi 32 bit
    uint64_t retv;
    memcpy((void*)&retv, (void*)&most_sign , 4);
    // Converto prima perchè altrimenti somma 4*sizeof(retv) = 32 byte
    memcpy(((char*)(&retv) + 4), (void*)&least_sign, 4); 
    return retv;
}
uint64_t ntohll(uint64_t num)
{
    return htonll(num);
}
#endif

// manda size byte di msg via sock
// ritorna 1 se eseguita con successo
int send_msg(int sock, void* msg, size_t size)
{
    ssize_t sent = send(sock, msg, size, 0);
    if(sent < 0)
    {
        perror("\n>! Errore send");
        return 0;
    }
    if(sent < size)
    {
        printf("\n>! Errore send interlocutore:\n\tInviato %lu invece di %lu", sent, size);
        return 0;
    }
    return 1;
}

// riceve messaggio dal socket (argomento 1) un messaggio di dimensione (argomento 2)
void* get_msg(int sock, void *buf, size_t size)
{
    ssize_t recived = recv(sock, buf, size, 0);
    if(recived < 0)
    {
        perror("\n>! Errore recv");
        return NULL;
    }
    if(recived < size)
    {
        printf("\n>! Errore recv interlocutore:\n\tRicevuto %lu invece di %lu", recived, size);
        return NULL;
    }
    return buf;
}

int send_card(int socket, task_card_t *cc)
{
    size_t net_card = sizeof(*cc) - sizeof(cc->desc) + strlen(cc->desc);
    char buffer[net_card]; 
    unsigned dim = prepare_card(cc, buffer);
    char instr_to_server[2]; 
    char instr_from_server[2];
    get_msg(socket, instr_from_server, 2);
    instr_to_server[1] = dim;
    send_msg(socket, instr_to_server, 2);
    send_msg(socket, buffer, dim + sizeof(*cc) - sizeof(cc->desc));
    return 1; // TODO error handling
}

task_card_t* recive_card(int socket, size_t dim_desc_card)
{
    printf("dbg> [recive_card] dim: %lu\n", dim_desc_card);
    printf("dbg> [recive_card] malloc \n");
    task_card_t* card = (task_card_t*)malloc(sizeof(task_card_t));
    // preparo il buffer per la card in versione network
    char net_card[dim_desc_card + sizeof(task_card_t) - sizeof(char*)]; 
    printf("dbg> [recive_card] get_msg \n");
    get_msg(socket, net_card, dim_desc_card + sizeof(task_card_t) - sizeof(char*));
    printf("dbg> [recive_card] dim prima di unprepare: %lu\n", dim_desc_card);
    unprepare_card(card, net_card, dim_desc_card); // alloca la descrizione
    return card;
}

