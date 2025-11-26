# Compilo con opzioni per debugging e per vedere tutti gli warning

all: utente lavagna

lavagna:
	gcc -o bin/lavagna -Wall -g src/lavagna*.c src/common*.c

utente: 
	gcc -o bin/utente -Wall -g src/utente*.c src/common*.c

clean:
	rm bin/*
