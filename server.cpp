#include "given.h"
#include "util.h"
#include <unordered_map>
using namespace std;

fstream output;

void print_time(){

    const auto p1=std::chrono::system_clock::now();
    long long elapsed=std::chrono::duration_cast<std::chrono::milliseconds>(p1.time_since_epoch()).count();
    float time=(float)elapsed/1000;
    output<<fixed<<setprecision(2)<<time<<": ";

    return;
}

int main(int argc,char* argv[]){
    assert(argc==2);

    uint16_t port_num;
    istringstream iss(argv[1]);
    iss>>port_num;

    output.open("server.log",ios::out);

    output<<"using port "<<port_num<<'\n';

    int listenfd=socket(AF_INET,SOCK_STREAM,0);
    if (listenfd<0) perror("failed to create a socket");

    sockaddr_in serv_addr,client_addr;
    serv_addr.sin_family=AF_INET;
    serv_addr.sin_addr.s_addr=htonl(INADDR_ANY);
    serv_addr.sin_port=htons(port_num);

    if (bind(listenfd,(sockaddr*)&serv_addr,sizeof(serv_addr))<0) perror("failed to bind");

    if (listen(listenfd,10)<0) perror("unable to listen");

    timeval timeout;
    timeout.tv_sec=30;
    timeout.tv_usec=0;

    unsigned c=sizeof(sockaddr_in);
    int client_fd;

    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(listenfd,&readfds);

    pid_t pid;
    string hostname;
    // receive client messages and execute
    // get the starting time
    auto s=std::chrono::system_clock::now();
    long long begin=std::chrono::duration_cast<std::chrono::milliseconds>(s.time_since_epoch()).count();

    unordered_map<string,int> counts;       // counts the number of transactions for each client

    int num=1;
    // start receiving message until no message is on the queue for more than 30 seconds
    while (1){
        if (select(listenfd+1,&readfds,NULL,NULL,&timeout)>0){    // if an incoming message arrives within 30 seconds
            // first get the hostname and pid of the client
            client_fd = accept(listenfd,(sockaddr*)&client_addr,&c);
            if (client_fd<0) perror("unable to accept message");

            int read_size=recv(client_fd,&pid,sizeof(pid_t),0);
            assert(read_size==sizeof(pid_t));

            hostent *hostName;
            in_addr ipv4addr;
            inet_pton(AF_INET,inet_ntoa(client_addr.sin_addr),&ipv4addr);
            hostName=gethostbyaddr(&ipv4addr,sizeof(ipv4addr),AF_INET);

            hostname=hostName->h_name;
            // modify for the output format
            hostname+='.';
            hostname+=to_string(pid);

            counts[hostname]++;        // increment the count of the current client
        }

        else{
            // no incoming messages within 30 seconds, prepare the close communication
            // get the end time
            s=std::chrono::system_clock::now();
            long long end=std::chrono::duration_cast<std::chrono::milliseconds>(s.time_since_epoch()).count();
            float elapsed=(float)(end-begin)/1000;
            num--;
            float average=(float)num/elapsed;

            output<<"SUMMARY\n";

            for (auto i=counts.begin();i!=counts.end();i++) output<<i->second<<" transactions from "<<i->first<<"\n";

            output<<average<<" transactions/sec "<<'('<<num<<'/'<<elapsed<<'\n';

            // shut down the connection
            shutdown(listenfd,SHUT_RD);
            close(listenfd);
            break;
        }

        // after getting the hostname and pid of the client
        int message;
        int read_size=recv(client_fd,&message,4,0);         // receives a message from the client
        print_time();
        output<<"# "<<num<<" (T "<<message<<") from "<<hostname<<'\n';

        Trans(message);

        print_time();
        output<<"# "<<num<<" (Done) from "<<hostname<<'\n';
        
        int write_size=write(client_fd,&num,4);         // send the message back to client
        assert(write_size==4);

        num++;      
    }    
    
    return 0;
}