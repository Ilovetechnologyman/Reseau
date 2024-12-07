#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <string.h>
#include <stdio.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>


int main(){
    struct sockaddr_in server_addr;
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); 
    server_addr.sin_port = htons(1234);
    char buffer[1024];
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("ERROR connecting");
        close(sockfd);
        exit(1);
    }
    for(int i=0;i<1000;i++){
        send(sockfd,"ECHO",6,0);
        int bytes_received = recv(sockfd, buffer, 1023, 0);
        buffer[bytes_received] = '\0';
        printf("Server : %s \n",buffer);
    }
    close(sockfd);
}