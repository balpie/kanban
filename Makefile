# Compilo con opzioni per debugging e per vedere tutti gli warning

all: utente lavagna

bin:
	mkdir -p bin; mkdir log

lavagna: bin
	gcc -o bin/lavagna -Wall -g src/lavagna*.c src/common*.c -lpthread

utente: bin
	gcc -o bin/utente -Wall -g src/utente*.c src/common*.c -lpthread

doc: 
	mkdir -p /tmp/latex; pdflatex -aux-directory=/tmp/latex documentazione.tex -o documentazione.pdf 

clean:
	rm bin/*
