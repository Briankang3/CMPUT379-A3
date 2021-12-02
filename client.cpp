#include "util.h"
#include "given.h"
using namespace std;

fstream output;

// this function gets UNIX epoch time at the moment when it is called
double get_time(){
    
    auto p1=std::chrono::system_clock::now();
    double elapsed=std::chrono::duration_cast<std::chrono::milliseconds>(p1.time_since_epoch()).count();
    double time=elapsed/1000;

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
    string filename=host+to_string(pid);        // now filename should be the expected one
    output.open(&filename[0],ios::out);         // create the log file for the client

    output<<"Using port "<<port_num<<'\n';
    output<<"Using server address "<<ip_address<<'\n';
    output<<"Host "<<filename<<'\n';

    int client_fd=socket(AF_INET,SOCK_STREAM,0);      // acquire a file descriptor to write on
    if (client_fd<0) perror("unable to obtain the client socket");

    sockaddr_in server;
    server.sin_addr.s_addr=inet_addr(ip_address.c_str());
    server.sin_family=AF_INET;
    server.sin_port=htons(port_num);

    if (connect(client_fd,(sockaddr*)&server,sizeof(server))<0) perror("cannot to connect to the server");

    string cmd;
    int count=0;            // will be the total number of transactions sent from this client
    while (cin>>cmd){
        string sub=cmd.substr(1,cmd.size());
        istringstream iss(sub);
        uint32_t to_write;
        iss>>to_write;
        
        if (cmd[0]=='S'){
            output<<"sleep "<<to_write<<" units\n";

            shutdown(client_fd,SHUT_WR);     // temporarily shuts down connection with the server
            close(client_fd);                // close the file, return it to system kernal

            Sleep(to_write);                 // sleep

            client_fd=socket(AF_INET,SOCK_STREAM,0);        // acquire a new file descriptor as client socket to write on
            if (client_fd<0) perror("unable to obtain the client socket");
            
            // establish a new connection with the server
            if (connect(client_fd,(sockaddr*)&server,sizeof(server))<0) perror("cannot to connect to the server");
        }

        else{
            count++;

            // first send the pid of the current client process
            uint32_t write_size=send(client_fd,&pid,sizeof(pid_t),0);
            while (write_size<sizeof(pid_t)){     // if not sent completely
                write_size+=send(client_fd,(char*)&pid+write_size,sizeof(pid_t)-write_size,0);
            }
            
            // send the parameter for Trans()
            double time=get_time();
            output<<fixed<<setprecision(2)<<time<<": ";
            output<<"send "<<"(T  "<<to_write<<')'<<'\n';

            write_size=send(client_fd,&to_write,4,0);
            while (write_size<4){              // if not sent completely
                write_size+=send(client_fd,(char*)&to_write+write_size,4-write_size,0);
            }
            
            // wait for reply from the server
            uint32_t reply;
            uint32_t reply_size=recv(client_fd,&reply,sizeof(reply),0);
            while (reply_size<4){             // if not received completely
                reply_size+=recv(client_fd,(char*)&reply+reply_size,4-reply_size,0);
            }
            
            time=get_time();
            output<<fixed<<setprecision(2)<<time<<": ";
            output<<"recv "<<"(D  "<<reply<<')'<<'\n';
        }
    }

    output<<"Sent "<<count<<" transactions\n";
    shutdown(client_fd,SHUT_WR);      // shut down the connection for writing, WHILE data sent can still be read by the server
    close(client_fd);                 // close the file and return it to system kernal.

    return 0;
}
