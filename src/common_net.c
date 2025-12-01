#include <string.h>
#include <stdio.h>
#include "../include/common_net.h"

// Usando il preprocessore funziona sia per macchine che
// adottano little endian che per macchine che adottano 
// big endian, ammesso che usino gcc
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
uint64_t htonll(uint64_t num)
{
    uint32_t last32 = htonl((uint32_t)(num >> 32));// hton ultimi 32 bit
    uint32_t first32 = htonl((uint32_t)num);// hton primi 32 bit
    uint64_t retv;
    memcpy((void*)&retv, (void*)&first32, 4);
    // Converto prima perchè altrimenti somma 4*sizeof(retv) = 32 byte
    memcpy(((char*)(&retv) + 4), (void*)&last32, 4); 
    return retv;
}
uint64_t ntohll(uint64_t num)
{
    return htonll(num);
}
#else
uint64_t htonll(uint64_t num)
{
    return num;
}
uint64_t ntohll(uint64_t num)
{
    return num;
}
#endif

// manda size byte di msg via sock
// ritorna 1 se eseguita con successo
int send_msg(int sock, void* msg, size_t size)
{
    ssize_t sent = send(sock, msg, size, 0);
    if(sent < 0)
    {
        perror("Errore send");
        return 0;
    }
    if(sent < size)
    {
        printf("Errore recv interlocutore");
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
        perror("Errore send");
        return NULL;
    }
    if(recived < size)
    {
        printf("Errore recv interlocutore");
        return NULL;
    }
    return buf;
}
