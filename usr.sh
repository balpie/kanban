#!/bin/bash
#
# Per lanciare lo script è necessario che lavagna
# sia già in esecuzione

USR_FILE="./.curr_users"

if ! [[ -a $USR_FILE ]]
then
    echo 5679 > $USR_FILE
fi

# prendo la porta da utilizzare dal file generato dallo script
port=$(tail -n 1 $USR_FILE)

# mi scrivo la porta da usare nella successiva esecuzione 
echo $(( $port + 1 )) > $USR_FILE

# Creo utente con una porta non utilizzata
./utente $port
