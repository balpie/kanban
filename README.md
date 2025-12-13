# Progetto per il corso di Reti Informatiche    
Progetto compilato con `gcc`, testato su sistema operativo Debian trixie e Ubuntu 24.  
- Per **compilare** fare semplicemente `make`.  
- Per compilare la documentazione fare `make doc`. Dipende da `pandoc` e un qualsiasi latex engine.  

l'eseguibile `utente` funziona solo se `lavagna` è già in esecuzione. Sia per `utente` che per `lavagna`
può essere usato l'argomento `-d`, che farà si che i log di errore siano visibili insieme ai messaggi
sullo `stdout`.  

I comandi disponibili per `lavagna` sono i seguenti: 
    - `show_lavagna` mostra a video la lavagna
    - `see_connected` mostra a video la lista di utenti connessi e le relative porte

I comandi disponibili per `utente` sono i seguenti: 
    - `show_lavagna`: aggiorna la lavagna e la mostra a video
    - `create_card`: crea una card e la manda alla lavagna
