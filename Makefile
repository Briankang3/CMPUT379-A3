all: server client MAN

server: server.o given.o
	g++ -O -pthread server.o given.o -o server

server.o: server.cpp given.h util.h
	g++ -O -c -pthread server.cpp -std=c++2a

client: client.o given.o
	g++ -O client.o given.o -o client

client.o: client.cpp given.h util.h
	g++ -O -c client.cpp -std=c++2a

given.o: given.cpp given.h
	g++ -O -c given.cpp -std=c++2a

MAN: this.man
	groff -Tascii -man this.man | col -b > man.txt
	libreoffice --convert-to "pdf" man.txt
