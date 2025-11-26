- Sarà tenuto in considerazione se il layout della lavagna verrà diviso effettivamente in colonne?
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
BASTA CHE LA DIVISIONE IN COLONNE SIA CONCETTUALE

- Move card va eseguita anche a seguito del `pong_lavagna` ?
- Se uso udp devo fare rdt?
- `SEND_USER_LIST` Quando va usata? a chi deve essere mandata la lista se 
   in `AVALIABLE_CARD` devo implementare la stessa cosa in modo leggermente diverso?
- Posso compilare moduli in comune tra client e lavagna?
- Lavagna si deve ricordare della sessione precedente (file)?
- per il formato `time_t` non è definito nessuno standard di codifica.
  è considerato corretto assumere che la codifica sia la stessa per tutti gli utenti e la lavagna?  
  perchè in questo caso lo è ma in un caso d'uso reale non si potrebbe fare affidamento su 
  `time_t` per comunicare un'istante nel tempo tra macchine diverse
