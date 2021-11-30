#include "given.h"
#include "util.h"
#include <unordered_map>
#include <pthread.h>
#include <csignal>
#include <unistd.h>
using namespace std;

fstream output;
int listenfd;
unordered_map<string,int> counts;       // counts the number of transactions for each client
long long start;
uint32_t num=1;

float get_time(){

    auto p1=std::chrono::system_clock::now();
    long long elapsed=std::chrono::duration_cast<std::chrono::milliseconds>(p1.time_since_epoch()).count();
    float time=(float)elapsed/1000;

    return time;
}

void* timer(void* arg){
    pid_t pid=*(pid_t*)arg;
    
    sleep(30);

    kill(pid,SIGINT);

    return nullptr;
}

void handler(int sig){
    if (sig==SIGINT){
        // no incoming messages within 30 seconds, prepare the close communication
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
        shutdown(listenfd,SHUT_RD);
        close(listenfd);

        exit(0);
    }
}

int main(int argc,char* argv[]){
    signal(SIGINT,handler);
    
    uint32_t port_num;
    istringstream iss(argv[1]);
    iss>>port_num;

    output.open("server.log",ios::out);

    output<<"using port "<<port_num<<'\n';

    listenfd=socket(AF_INET,SOCK_STREAM,0);
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
    start=std::chrono::duration_cast<std::chrono::milliseconds>(s.time_since_epoch()).count();
    
    pthread_t t;
    while (1){
        client_fd = accept(listenfd,(sockaddr*)&client_addr,(socklen_t*)&c);
        if (num>1) pthread_kill(t,SIGKILL);
        
        // start receiving message until no message is on the queue for more than 30 seconds
        while (1){  
            // first get the hostname and pid of the client
            uint32_t read_size=recv(client_fd,&pid,sizeof(pid_t),0);
            while (read_size<sizeof(pid_t)) read_size+=recv(client_fd,(char*)&pid+read_size,sizeof(pid_t)-read_size,0);

            if (pid==-1) break;
            
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
            while (read_size<4) read_size+=recv(client_fd,(char*)&message+read_size,4-read_size,0);
            
            float time=get_time();
            output<<fixed<<setprecision(2)<<time<<": ";
            output<<"# "<<num<<" (T "<<message<<") from "<<hostname<<'\n';

            Trans(message);

            time=get_time();
            output<<fixed<<setprecision(2)<<time<<": ";
            output<<"# "<<num<<" (Done) from "<<hostname<<'\n';
            
            uint32_t write_size=send(client_fd,&num,sizeof(num),0);         // send the message back to client
            while (write_size<sizeof(num)) write_size+=send(client_fd,(char*)&num+write_size,sizeof(num)-write_size,0);
            
            num++;
        }
        
        pid=getpid();
        pthread_create(&t,nullptr,&timer,&pid);
    }  
    
    return 0;
}