#define _GNU_SOURCE
#include <sched.h>
#include <unistd.h> 
#include <stdio.h> 
#include <sys/socket.h> 
#include <stdlib.h> 
#include <netinet/in.h> 

#define PORT 8080

unsigned long limit=18;

void Init(){
    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(0, &mask);
    if (sched_setaffinity(0, sizeof(cpu_set_t), &mask) == -1) {
        perror("sched_setaffinity");
    }
    setvbuf(stdout, NULL, _IONBF, 0);
}

void CheckPassword(unsigned x){
    char flag[]="CTF-BR{eZ_PaRt_Ok}CTF-BR{not_so_write_only_now}";
    register unsigned offset =x/8;
    register unsigned suboffset =x%8;
    if(offset<limit){
        if((flag[offset] >> (suboffset) )&1){
            getenv("BitHit!");
        }     
    }

}

void MainLoop(int socket){
	unsigned index;
	size_t len;
    printf("connection started\n");
	while (1){
		if (read(socket,&index,sizeof(index))!=sizeof(index)){
		  break;
		}
		CheckPassword(index);
	}
}

int CreateServer(){
    int server_fd; 
    struct sockaddr_in address; 
    int opt = 1; 
    int addrlen = sizeof(address); 


    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) 
    { 
        perror("socket failed"); 
        exit(EXIT_FAILURE); 
    } 
       
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, 
                                                  &opt, sizeof(opt))) 
    { 
        perror("setsockopt"); 
        exit(EXIT_FAILURE); 
    } 
    address.sin_family = AF_INET; 
    address.sin_addr.s_addr = INADDR_ANY; 
    address.sin_port = htons( PORT ); 
       
    if (bind(server_fd, (struct sockaddr *)&address,  
                                 sizeof(address))<0) 
    { 
        perror("bind failed"); 
        exit(EXIT_FAILURE); 
    } 
    if (listen(server_fd, 3) < 0) 
    { 
        perror("listen"); 
        exit(EXIT_FAILURE); 
    } 
    return server_fd;
}

int ResetConn(int socket){
    int new_socket;
    struct sockaddr_in address; 
    int addrlen = sizeof(address); 
    if ((new_socket = accept(socket, (struct sockaddr *)&address,  
                       (socklen_t*)&addrlen))<0) 
    { 
        perror("accept"); 
        exit(EXIT_FAILURE); 
    }
    return new_socket; 

}

int main() 
{ 
    int server;
    int client;
    Init();

    printf("Welcome to write only password manager! (the password is already written).\n");
    server=CreateServer();
    while (1){
      client=ResetConn(server);
      MainLoop(client); 
    }
    return 0; 
} 
