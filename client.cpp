#include "util.h"
#include "given.h"
using namespace std;

fstream output;

float get_time(){

    auto p1=std::chrono::system_clock::now();
    long long elapsed=std::chrono::duration_cast<std::chrono::milliseconds>(p1.time_since_epoch()).count();
    float time=(float)elapsed/1000;

    return time;
}

int main(int argc,char* argv[]){
    assert(argc==3);
    
    uint32_t port_num;
    string ip_address=(string)argv[2];
    istringstream iss(argv[1]);
    iss>>port_num;

    char hostname[20];
    if (gethostname(hostname,20)<0) cout<<"cannot obtain client hostname\n";
    string host=(string)hostname+'.';

    pid_t pid=getpid();
    string filename=host+to_string(pid);
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
        uint32_t to_write;
        iss>>to_write;
        
        if (cmd[0]=='S'){
            output<<"sleep "<<to_write<<" units\n";

            Sleep(to_write);
        }

        else{
            count++;

            // send the pid of the current client process
            uint32_t write_size=send(client_fd,&pid,sizeof(pid_t),0);
            while (write_size<sizeof(pid_t)) write_size+=send(client_fd,(char*)&pid+write_size,sizeof(pid_t)-write_size,0);

            // send the parameter for Trans()
            float time=get_time();
            output<<fixed<<setprecision(2)<<time<<": ";
            output<<"send "<<"(T  "<<to_write<<')'<<'\n';

            write_size=send(client_fd,&to_write,4,0);
            while (write_size<4) write_size+=send(client_fd,(char*)&to_write+write_size,4-write_size,0);

            // wait for reply from the server
            uint32_t reply;
            uint32_t reply_size=recv(client_fd,&reply,sizeof(reply),0);
            while (reply_size<4) reply_size+=send(client_fd,(char*)&reply+reply_size,4-reply_size,0);
            
            time=get_time();
            output<<fixed<<setprecision(2)<<time<<": ";
            output<<"recv "<<"(D  "<<reply<<')'<<'\n';
        }
    }

    pid=-1;
    uint32_t write_size=send(client_fd,&pid,sizeof(pid_t),0);
    while (write_size<sizeof(pid_t)) write_size+=send(client_fd,(char*)&pid+write_size,sizeof(pid_t)-write_size,0);

    output<<"Sent "<<count<<" transactions\n";
    shutdown(client_fd,SHUT_WR);
    close(client_fd);

    return 0;
}
