# Documentazione progetto reti informatiche
## Strutture dati
### Attività
La struttura dati per le attività è di tipo `task_card_t`. Le proprietà sono:   
- `id`: su 8 bit senza segno, in quanto ho assunto che 256 come limite massimo per il numero di card fosse più che sufficiente.   
- `last_modified`: un timestamp, in secondi passati dall'epoch. Non ho usato direttamente `time_t` in quanto la sua codifica non è standard. Un vantaggio di questo approccio è che il timestamp in questo modo è memorizzato in modo relativamente compatto (8 byte). 
- `desc`: una stringa di `MAX_DIM_DESC` bit al massimo

### Lavagna  
**Per gli utenti** è una linked list di `task_card_t`. L'inserimento all'interno della lavagna è ordinato rispetto alla colonna per facilitare la stampa in ordine per colonne. 

### Connessioni

