#!/bin/bash

USR_FILE="./.curr_users"

if ! [[ -a $USR_FILE ]]
then
    echo 5679 > $USR_FILE
fi

port=$(tail -n 1 $USR_FILE)
echo $(( $port + 1)) > $USR_FILE
echo creo utente con porta $port 
./bin/utente $port 
