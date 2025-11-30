#include "../include/common.h"
#include "../include/lavagna.h"
#include "../include/lavagna_net.h"
#include <stdio.h>
#include <pthread.h>

connection_l lista_connessioni;

void* prompt_cycle()
{
    char cmd;
    do{
        cmd = prompt_line("lavagna");
        switch(cmd)
        {
            case CMD_NOP:
                break;
            case CMD_INVALID:
                printf("Comando inesistente\n");
                break;
            case CMD_STAMPA_UTENTI_CONNESSI:
                pthread_mutex_lock(&lista_connessioni.m);
                stampa_utenti_connessi(lista_connessioni.head);
                pthread_mutex_unlock(&lista_connessioni.m);
                break;
        }
    }while(cmd != CMD_QUIT);
    //TODO Fai effettivamente...
    printf("FINE!!!\n");  
    return NULL;
}

int main() // main thread: listener
{
    pthread_mutex_init(&lista_connessioni.m, 0);
    struct sockaddr_in listener_addr;
    struct sockaddr_in new_connection;
    int sock_listener = init_listener(&listener_addr);
    int new_sock;
    pthread_t prompt_thread;
    pthread_create(&prompt_thread, NULL, prompt_cycle, NULL);
    int count = 0;
    while(1)
    {
        unsigned int len_new = sizeof(new_connection);
        new_sock = accept(sock_listener, (struct sockaddr*)&new_connection, &len_new);
        pthread_mutex_lock(&lista_connessioni.m);
        insert_connection(&(lista_connessioni.head), &new_connection, new_sock);
        pthread_mutex_unlock(&lista_connessioni.m);
        count++;
    }
    return 0;
}   
