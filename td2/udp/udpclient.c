#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>



int main(){
    int sock = socket(AF_INET,SOCK_DGRAM,0);
     if(sock == -1){
        perror("socket()");
        exit(errno);
    }
    struct sockaddr_in serveraddr;
    memset(&serveraddr,0,sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = inet_addr("127.0.0.1"); 
    serveraddr.sin_port = htons(1234);
    char *buffer = "PING";
    char buf[1024];
    char *reception = malloc(sizeof(char *)*1024);
    int len = sizeof(serveraddr);
    while(1){
        sendto(sock,buffer,len,0,(const struct sockaddr *) &serveraddr,len);
        int n =recvfrom(sock,buf,1023,MSG_WAITALL,(struct sockaddr * restrict) &serveraddr,(socklen_t *) &len);
        buf[n] = '\0';
        printf("%s \n",buf);
    }
    free(reception);
}    
