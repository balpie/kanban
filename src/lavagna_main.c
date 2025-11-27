#include "../include/common.h"
#include "../include/lavagna.h"
#include "../include/lavagna_net.h"
#include <stdio.h>

connection_l lista_connessioni;

int main()
{
    /* 
     * da fare una sola volta
     */
    pthread_mutex_init(&lista_connessioni.m, 0);
    struct sockaddr_in listener_addr;
    struct sockaddr_in new_connection;
    int sock_listener = init_listener(&listener_addr);
    int new_sock;
    /*
     * da fare nel thread
     */
    int count = 0;
    while(1)
    {
        unsigned int len_new = sizeof(new_connection);
        printf("[main]accept:\n");
        new_sock = accept(sock_listener, (struct sockaddr*)&new_connection, &len_new);
        pthread_mutex_lock(&lista_connessioni.m);
        insert_connection(&(lista_connessioni.head), &new_connection, new_sock);
        pthread_mutex_unlock(&lista_connessioni.m);
        count++;
        printf("Connessi: %d\n", count);
        printf("Premi invio per uscire\n");
        getchar();
    }
    return 0;
}   

