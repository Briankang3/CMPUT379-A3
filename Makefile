all: server client MAN

server: server.o given.o
	g++ server.o given.o -o server

server.o: server.cpp given.h util.h
	g++ -c server.cpp -std=c++2a

client: client.o given.o
	g++ client.o given.o -o client

client.o: client.cpp given.h util.h
	g++ -c client.cpp -std=c++2a

given.o: given.cpp given.h
	g++ -c given.cpp -std=c++2a

MAN: this.man
	groff -Tascii -man this.man | install /dev/stdin man.txt
	libreoffice --convert-to "pdf" man.txt
