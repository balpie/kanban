# Compilo con opzioni per debugging e per vedere tutti gli warning

all: utente lavagna

bin:
	mkdir -p bin; mkdir log

lavagna: bin
	gcc -o bin/lavagna -Wall -g src/lavagna*.c src/common*.c -lpthread

utente: bin
	gcc -o bin/utente -Wall -g src/utente*.c src/common*.c -lpthread

doc: 
	pandoc -o documentazione.pdf documentazione.md

clean:
	rm bin/*
