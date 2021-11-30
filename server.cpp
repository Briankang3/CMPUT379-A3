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

    uint32_t port_num;
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

    if (bind(listenfd,(sockaddr*)&serv_addr,sizeof(serv_addr))<0) perror("failed to bind");     // binds the socket with the ip address

    if (listen(listenfd,10)<0) perror("unable to listen");

    uint32_t c=sizeof(sockaddr_in);
    int client_fd;

    pid_t pid;
    string hostname;
    // receive client messages and execute
    // get the starting time
    auto s=std::chrono::system_clock::now();
    long long begin=std::chrono::duration_cast<std::chrono::milliseconds>(s.time_since_epoch()).count();

    unordered_map<string,int> counts;       // counts the number of transactions for each client

    uint32_t num=1;
    // start receiving message until no message is on the queue for more than 30 seconds
    while (1){
        /*
        timeval timeout;
        timeout.tv_sec=20;
        timeout.tv_usec=0;
        
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(listenfd,&readfds);

        cout<<timeout.tv_sec<<"seconds remaining before\n";
        int S=select(listenfd+1,&readfds,NULL,NULL,&timeout);
        cout<<timeout.tv_sec<<"seconds remaining after\n";
        */
        while (1){
            int S=1;
            if (S==-1) perror("failed to select");
            
            else if (S>0){    // if an incoming message arrives within 30 seconds
                // first get the hostname and pid of the client
                cout<<"start of a new iteration\n";
                client_fd = accept(listenfd,(sockaddr*)&client_addr,(socklen_t*)&c);
                if (client_fd<0) perror("unable to accept message");
                cout<<"accepted a new request\n";

                uint32_t read_size=recv(client_fd,&pid,sizeof(pid_t),0);
                assert(read_size==sizeof(pid_t));
                cout<<"received pid\n";
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
                read_size=recv(client_fd,&message,4,0);         // receives a message from the client
                cout<<"received message\n";
                print_time();
                output<<"# "<<num<<" (T "<<message<<") from "<<hostname<<'\n';

                Trans(message);

                print_time();
                output<<"# "<<num<<" (Done) from "<<hostname<<'\n';
                
                cout<<num<<" is to be sent\n";
                uint32_t write_size=send(client_fd,&num,sizeof(num),0);         // send the message back to client
                cout<<num<<" was just sent"<<'\n';
                assert(write_size==sizeof(num));
                cout<<"end of an iteration\n";
                num++;
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

                output<<average<<" transactions/sec "<<'('<<num<<'/'<<elapsed<<')'<<'\n';

                // shut down the connection
                shutdown(listenfd,SHUT_RD);
                close(listenfd);
                break;
            }
        }      
    }    
    
    return 0;
}