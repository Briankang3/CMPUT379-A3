# CMPUT379-A3
Files: given.cpp & given.h: Provided on eclass, but I modified them in adaptation of c++.
       client.cpp: source code for the client.
       server.cpp: source code for the server.
       util.h: header file for client.cpp and server.cpp.
       this.man: source file for the manual page.

Compile: make all
run: (i.e.) 
	./server 5000
	./client 5000 127.0.0.1 <input.txt

Assumption:
	1. server(the executable file) should be running prior to client(the executable file).
	2. client(the executable) should be running before server(the executable file) times out.

Summary of approach to solve the problems:
	1. Once server starts running, it creates a socket(listenfd) that does not get destroyed throughout the execution.
	2. Bind listenfd to the internet.
	3. Limit the size of pending queue to 10 messages.
	
