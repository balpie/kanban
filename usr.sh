#!/bin/bash
# se passato argomento f sono il primo

USR_FILE="./.curr_users"

if [[ $1 == f ]]
then
    echo 5679 > $USR_FILE
fi

port=$(tail -n 1 $USR_FILE)
echo creo utente con porta $port 
./bin/utente $port -d
echo $(( $port + 1)) >> $USR_FILE
