#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "../include/common_net.h"

#define MAX_QUEUE 10

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
    ssize_t sent = 0;
    do
    {
        ssize_t offs = send(sock, (char*)msg + sent, size - sent, 0);
        sent += offs;
        if(sent < 0)
        {
            perror("\n>! Errore send");
            return 0;
        }
    }while(sent < size);
    return 1;
}

// ritorna il socket listener o -1 al fallimento
int init_listener(struct sockaddr_in* server_addr, uint16_t port)
{
    int listener = socket(AF_INET, SOCK_STREAM, 0);
    if(listener < 0)
    {
        return -1;
    }
    memset(server_addr, 0, sizeof(struct sockaddr_in));
    server_addr->sin_addr.s_addr = INADDR_ANY;
    server_addr->sin_family = AF_INET;
    server_addr->sin_port = htons(port);
    ERR("init_listener: inizializzo listener con porta: %d\n", port);
    if(bind(listener, (struct sockaddr*)server_addr, sizeof(*server_addr)) < 0)
    {
        ERR("init_listener errore bind");
        perror(NULL);
        exit(-1);
    }
    if(listen(listener, MAX_QUEUE) < 0)
    {
        ERR("init_listener errore listen");
        perror(NULL);
        exit(-1);
    }
    return listener;
}


// riceve messaggio dal socket (argomento 1) un messaggio di dimensione (argomento 2)
int get_msg(int sock, void *buf, size_t size)
{
    ssize_t recived = 0;
    do
    {
        ssize_t offs = recv(sock,(char*) buf, size - recived, 0);
        recived+=offs;
        if(recived == 0)
        {
            return RTR_CONN_CLOSE;
        }
        if(recived < 0)
        {
            if(errno == EWOULDBLOCK)
            {
                // se l'errore è questo la controparte ha tardato a mandare il messaggio
                return -1;
            }
            return 0;
        }
    }while (recived < size);
    return 1;
}

int send_card(int socket, task_card_t *cc)
{
    size_t net_card = sizeof(*cc) - sizeof(cc->desc) + strlen(cc->desc);
    char buffer[net_card]; 
    unsigned dim = prepare_card(cc, buffer);
    char instr_to_server[2]; 
    instr_to_server[0] = INSTR_NEW_CARD;
    instr_to_server[1] = strlen(cc->desc);
    send_msg(socket, instr_to_server, 2);
    send_msg(socket, buffer, dim + sizeof(*cc) - sizeof(cc->desc));
    return 1; // TODO error handling
}

task_card_t* recive_card(int socket, size_t dim_desc_card)
{
    task_card_t* card = (task_card_t*)malloc(sizeof(task_card_t));
    // preparo il buffer per la card in versione network
    char net_card[dim_desc_card + sizeof(task_card_t) - sizeof(char*)]; 
    get_msg(socket, net_card, dim_desc_card + sizeof(task_card_t) - sizeof(char*));
    unprepare_card(card, net_card, dim_desc_card); // alloca la descrizione
    return card;
}

