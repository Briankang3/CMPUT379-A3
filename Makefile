all: server client

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