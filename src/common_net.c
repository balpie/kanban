#include <stdio.h>
#include "../include/common_net.h"

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
