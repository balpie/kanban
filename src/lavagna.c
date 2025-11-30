#include "../include/common.h"
#include "../include/lavagna.h"
#include "../include/lavagna_net.h"
#include <stdio.h>

void stampa_utenti_connessi(connection_l_e *head)
{
    int count = 0;
    while(head!= NULL)
    {
        printf("%d)Port %u\n", ++count, ntohs(head->indirizzo.sin_port));
        head = head->next;
    }
}
