#include "util.h"
#include "given.h"
#include <cstdlib>
#include <cerrno>
#include <iostream>
#include <cstdio>
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
    assert(argc==3);
    
    uint16_t port_num;
    string ip_address=(string)argv[2];
    istringstream iss(argv[1]);
    iss>>port_num;

    string hostname;
    if (gethostname(&hostname[0],10)<0) perror("cannot obtain client host name");

    pid_t pid=getpid();
    string filename=hostname+to_string(pid);
    cout<<"file name is "<<filename<<'\n';
    output.open(&filename[0],ios::out);

    output<<"Using port "<<port_num<<'\n';
    output<<"Using server address "<<ip_address<<'\n';
    output<<"Host "<<filename<<'\n';

    int client_fd=socket(AF_INET,SOCK_STREAM,0);
    if (client_fd<0) perror("unable to obtain the client socket");

    sockaddr_in server;
    server.sin_addr.s_addr=inet_addr(ip_address.c_str());
    server.sin_family=AF_INET;
    server.sin_port=htons(port_num);

    if (connect(client_fd,(sockaddr*)&server,sizeof(server))<0) perror("cannot to connect to the server");

    string cmd;
    int count=0;
    while (cin>>cmd){
        string sub=cmd.substr(1,cmd.size());
        istringstream iss(sub);
        int to_write;
        iss>>to_write;

        if (cmd[0]=='S'){
            output<<"sleep "<<to_write<<" units\n";

            Sleep(to_write);
        }

        else{
            count++;

            // first send the pid of the current client process
            int write_size=send(client_fd,&pid,sizeof(pid_t),0);
            assert(write_size==sizeof(pid_t));

            // send the parameter for Trans()
            print_time();
            output<<"send "<<"(T  "<<to_write<<')'<<'\n';

            write_size=send(client_fd,&to_write,4,0);
            assert(write_size==4);

            // wait for reply from the server
            int reply;
            int reply_size=recv(client_fd,&reply,4,0);
            if (reply_size!=4){
                cout<<"reply_size=="<<reply_size<<'\n';
                const char* buffer=hstrerror(errno); // get string message from errno, XSI-compliant version
                cout<<"errono is "<<string(buffer)<<'\n';

                abort();
            }

            print_time();
            output<<"recv "<<"(D  "<<reply<<')'<<'\n';
        }
    }

    output<<"Sent "<<count<<" transactions\n";
    shutdown(client_fd,SHUT_WR);
    close(client_fd);

    return 0;
}