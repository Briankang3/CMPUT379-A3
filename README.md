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
	Server side:
	1. Once server starts running, it creates a non-blocking socket(listenfd) that does not get destroyed throughout the execution. 
	2. Bind listenfd to the internet.
	3. Limit the size of pending queue to 10 messages.
	4. Create an array(vector) of (struct pollfd) for poll() system call. Initially only listenfd is added to the array to accept connection requests.
	5. Once a new connection is accepted, the client socket(client_fd) is added to the array.
	6. If there is no connection requests or incoming messages from established connections, shut down the server socket.

	Client side:
	1. While not sleeping, for each transaction, send the process id and the integer to the server.
	2. Before sleeping, shut down the socket for writing and close the socket.
	3. Once it wakes up, acquire a new socket to re-establish connection with the server.
	4. If no more input, shut down the socket for writing and close the socket.
	
