# Compilo con opzioni per debugging e per vedere tutti gli warning

all: utente lavagna log

log:
	mkdir -p log

lavagna: 
	gcc -o lavagna -Wall -g src/lavagna*.c src/common*.c -lpthread

utente: 
	gcc -o utente -Wall -g src/utente*.c src/common*.c -lpthread

doc: 
	mkdir -p /tmp/latex; pdflatex -aux-directory=/tmp/latex documentazione.tex -o documentazione.pdf 

clean:
	rm log/* utente lavagna
