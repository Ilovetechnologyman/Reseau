#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>


void stop(char *s){
    perror(s);
    exit(1);
}
int main(){
    int sock = socket(AF_INET,SOCK_DGRAM,0);
    if(sock == -1){
        perror("socket()");
        exit(errno);
    }
    struct sockaddr_in serveraddr,cliaddr;
    memset(&serveraddr,0,sizeof(serveraddr));
    memset(&cliaddr,0,sizeof(cliaddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = inet_addr("127.0.0.1"); 
    serveraddr.sin_port = htons(1234);    
    if(bind(sock,(const struct sockaddr *) &serveraddr,sizeof(serveraddr))){
        perror("ERROR ON binding");
        exit(1);
    }
    char buffer[1024];
    int len = sizeof(cliaddr);
    while (1){
        int n = recvfrom(sock,buffer,1023,MSG_WAITALL,(struct sockaddr * restrict) &cliaddr,(socklen_t *) &len);
        if (n < 0) {
            perror("recvfrom()");
            continue;  
        }
        buffer[n] = '\0';
        printf("Client : %s \n",buffer);
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(cliaddr.sin_addr), client_ip, INET_ADDRSTRLEN);
        printf("Émetteur : IP = %s , Port = %d\n",client_ip,ntohs(cliaddr.sin_port));
        sleep(1);
        char *msg = "BONG";
        int a = sizeof(cliaddr);
        sendto(sock, msg, a, 0, (const struct sockaddr *) &cliaddr, a);
        
    }
}