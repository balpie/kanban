Q Sarà tenuto in considerazione se il layout della lavagna verrà diviso effettivamente in colonne?
  se invece di dividere la lavagna in colonne la divido in sezioni? tipo 

        +-----------+
        |   TO DO   |
        +-----------+
        | - bla bla |
        | - bla bla |
        | - bla bla |
        | - bla bla |
        +-----------+
        |   DOING   |
        +-----------+
        | - bla bla |
        | - bla bla |
        +-----------+
        |    DONE   |
        +-----------+
        | - bla bla |
        | - bla bla |
        +-----------+

invece di ...

        +-----------+ +-----------+ +-----------+                            
        |   TO DO   | |   DOING   | |    DONE   |                            
        +-----------+ +-----------+ +-----------+                                               
        | - bla bla | | - bla bla | | - bla bla |                                               
        | bla bla   | | - bla bla | | - bla bla |                                               
        | bla bla   | +-----------+ +-----------+                                               
        | bla bla   |                                                 
        +-----------+ 
A: Basta che la divisione in colonne sia concettuale

Q: Move card va eseguita anche a seguito del `pong_lavagna` ?
Q: Se uso udp devo fare rdt?
A: Devi motivare tutto quello che fai
Q: `SEND_USER_LIST` Quando va usata? a chi deve essere mandata la lista se in `AVALIABLE_CARD` devo 
implementare la stessa cosa in modo leggermente diverso?
A: Deve essere coerente col numero di matricola
Q: Posso compilare moduli in comune tra client e lavagna?
Q: Lavagna si deve ricordare della sessione precedente (file)?
Q: Nelle specifiche dice minimo 10 card. Queste 10 card devono già essere presenti 
   all'inizio, o 10 è il minimo numero massimo di card presenti nella lavagna?
Q: per il formato `time_t` non è definito nessuno standard di codifica.
  è considerato corretto assumere che la codifica sia la stessa per tutti gli utenti e la lavagna?  
  perchè in questo caso lo è ma in un caso d'uso reale non si potrebbe fare affidamento su 
  `time_t` per comunicare un'istante nel tempo tra macchine diverse
Q: Ai client devo mandare anche i rispettivi indirizzi, nonostante siano sempre quello di loopback?

