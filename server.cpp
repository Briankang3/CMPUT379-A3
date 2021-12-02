#include "given.h"
#include "util.h"
#include <unordered_map>
#include <poll.h>
#include <vector>
#include <cerrno>
using namespace std;

// this function gets UNIX epoch time at the moment when it is called
double get_time(){
    
    auto p1=std::chrono::system_clock::now();
    double elapsed=std::chrono::duration_cast<std::chrono::milliseconds>(p1.time_since_epoch()).count();
    double time=elapsed/1000;
    
    return time;
}

int main(int argc,char* argv[]){
    assert(argc==2);

    fstream output;
    int listenfd;
    unordered_map<string,int> counts;       // counts the number of transactions for each client. "string" is the client hostname.
    uint32_t num=1;

    uint32_t port_num;
    istringstream iss(argv[1]);
    iss>>port_num;

    output.open("server.log",ios::out);     // creates the log file for the server

    output<<"using port "<<port_num<<'\n';

    listenfd=socket(AF_INET,SOCK_STREAM | SOCK_NONBLOCK,0);    // acquire a socket to listen to
    if (listenfd<0) perror("failed to create a socket");

    sockaddr_in serv_addr,client_addr;
    serv_addr.sin_family=AF_INET;
    serv_addr.sin_addr.s_addr=htonl(INADDR_ANY);
    serv_addr.sin_port=htons(port_num);

    if (bind(listenfd,(sockaddr*)&serv_addr,sizeof(serv_addr))<0) perror("failed to bind");     // binds the socket with the ip address

    if (listen(listenfd,10)<0) perror("unable to listen");     // set the maximum queue size to 10

    uint32_t c=sizeof(sockaddr_in);
    int client_fd;
    
    pid_t pid;      // will be the received process id of the client
    string hostname;    // will be the received client hostname

    // get the starting time
    auto s=std::chrono::system_clock::now();
    long long start=std::chrono::duration_cast<std::chrono::milliseconds>(s.time_since_epoch()).count();

    vector<pollfd> fds;      // an array of (struct pollfd) for poll() system call

    pollfd temp;             // create a (struct pollfd) for the listening socket file
    temp.events=POLLIN;      // poll() sniffs whether it is ready to be read without blocking
    temp.fd=listenfd;

    fds.push_back(temp);
    nfds_t nfds=1;        // will be fds.size()
    int timeout=30000;    // 30 seconds
    
    while (1){
        // Check if any of the files is ready to be read. Initially, it only checks the listening socket for connection requests.
        int ret=poll(&fds[0],nfds,timeout);           
        if (ret<0) perror("failed at poll()");

        else if (ret==0){
            // no incoming message or connection requests within 30 seconds
            // get the end time
            auto s=std::chrono::system_clock::now();
            long long end=std::chrono::duration_cast<std::chrono::milliseconds>(s.time_since_epoch()).count();
            float elapsed=(float)(end-start)/1000;
            num--;
            float average=(float)num/elapsed;

            output<<"SUMMARY\n";

            for (auto i=counts.begin();i!=counts.end();i++) output<<i->second<<" transactions from "<<i->first<<"\n";

            output<<average<<" transactions/sec "<<'('<<num<<'/'<<elapsed<<')'<<'\n';

            // shut down the connection
            shutdown(listenfd,SHUT_RD);     // shuts down for read, while buffered writing can continue.
            close(listenfd);                // close the listening socket and return it to system kernal.

            break;      // exits the while loop
        }

        else{        // receives connection requests or incoming messages.
            client_fd = accept(listenfd,(sockaddr*)&client_addr,(socklen_t*)&c);     // attemps to accept a new connection
            if (client_fd>=0){          // if there is a connection request, accept it.
                temp.fd=client_fd;
                temp.events=POLLIN;
                fds.push_back(temp);     // add it to the array of files to be poll()ed on
                nfds++;
            }

            // if there is no connection requests, the error message should be "EWOULDBLOCK"
            // however, other error might arise, which should be handled seperatly.
            else if (errno!=EWOULDBLOCK) perror("failed to accept a connection");

            // loop through each client socket file for transactions.
            for (int i=1;i<fds.size();i++){
                pollfd FD=fds[i];
                if (FD.revents & POLLIN){       // if it's ready to be read.
                    // first get the hostname and pid of the client
                    uint32_t read_size=recv(FD.fd,&pid,sizeof(pid_t),0);
                    if (read_size<0) perror("error receiving message");

                    // handles the case where the connection has been closed by the client.
                    else if (read_size==0){        
                        close(fds[i].fd);          // close the corresponding client socket file.

                        fds.erase(fds.begin()+i);    // remove the corresponding (struct pollfd) from the array.
                        nfds--;
                        
                        continue;
                    }

                    while (read_size<sizeof(pid_t)){     // if not received completely
                        read_size+=recv(FD.fd,(char*)&pid+read_size,sizeof(pid_t)-read_size,0);
                    }
                    
                    hostent *hostName;
                    in_addr ipv4addr;
                    inet_pton(AF_INET,inet_ntoa(client_addr.sin_addr),&ipv4addr);
                    hostName=gethostbyaddr(&ipv4addr,sizeof(ipv4addr),AF_INET);
                    
                    hostname=hostName->h_name;
                    // modify for the output format
                    hostname+='.';
                    hostname+=to_string(pid);
                    
                    counts[hostname]++;        // increment the count of the current client
                    
                    // after getting the hostname and pid of the client
                    uint32_t message;
                    read_size=recv(FD.fd,&message,4,0);         // receives a message from the client
                    if (read_size<0) perror("error receiving message");
                    while (read_size<4){      // if not received completely
                        read_size+=recv(FD.fd,(char*)&message+read_size,4-read_size,0);
                    }
                    
                    double time=get_time();
                    output<<fixed<<setprecision(2)<<time<<": ";
                    output<<"# "<<num<<" (T "<<message<<") from "<<hostname<<'\n';

                    Trans(message);       // process the transaction

                    time=get_time();
                    output<<fixed<<setprecision(2)<<time<<": ";
                    output<<"# "<<num<<" (Done) from "<<hostname<<'\n';
                    
                    uint32_t write_size=send(FD.fd,&num,sizeof(num),0);         // send the message back to client
                    while (write_size<sizeof(num)){        // if not sent completely.
                        write_size+=send(FD.fd,(char*)&num+write_size,sizeof(num)-write_size,0);
                    }
                    
                    num++;
                }
            }
        }
    }  
    
    return 0;
}