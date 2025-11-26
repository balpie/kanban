#ifndef LAVAGNA_H
#define LAVAGNA_H

/*
 * FAI UDP, TANTO C'E GIA PING E PONG 
 */
  
// MOVE_CARD:
// Sposta una card da una colonna ad un'altra
// Viene eseguita ogni volta che viene ricevuta 
// un'informazione da parte di un utente

// SHOW_LAVAGNA:
// Viene mostrata la lavagna, con le card assegnate ognuna alla giusta colonna

// SEND_USER_LIST:
// Manda la lista delle porte degli utenti 

// PING_USER:
// Invia un messaggio periodicamente agli utenti con una card in doing, 
// se questi non rispondono entro un timeout con pong_lavagna vengono considerati non connessi
// e la/le relative task finiscono in todo

// AVALIABLE_CARD:
// Comunica a tutti gli utenti la prima carta disponibile nella colonna
// todo, e la lista degli utenti presenti
// escluso l'utente a cui viene mandato.


#endif
