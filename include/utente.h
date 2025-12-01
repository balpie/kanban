#ifndef UTENTE_H
#define UTENTE_H
#include "../include/common.h"
  
// CREATE_CARD:
// Crea carta (con id, colonna, e descrizione prese da tastiera)
task_card_t* create_card(); // TODO: send card
  
// SHOW_LAVAGNA:
// Viene mostrata la lavagna, con le card assegnate ognuna alla giusta colonna

// PONG_LAVAGNA:
// Messaggio di risposta a PING_USER

// CHOSE_USER
// Messaggio da inviare tra utenti per decidere chi prenderà la card
// randomizzare il costo, card a costo minore, a parità di costo porta minore

// ACK_CARD
// L'utente a cui viene assegnata la card lo comunica alla lavagna
  
// CARD_DONE
// L'utente comunica alla lavagna la terminazione dell'attività
#endif
