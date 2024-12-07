#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <string.h>
#include <stdio.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>

int main(){
    struct sockaddr_in server_addr,clien_addr;

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); 
    server_addr.sin_port = htons(1234);

    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    if (listen(sockfd, 5) < 0) {
        perror("listen");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    socklen_t client_len = sizeof(clien_addr);
    int a = accept(sockfd, (struct sockaddr *) &clien_addr, &client_len);
    char buffer[1024];
    bzerro(buffer,1024);
    for(int i = 0;i<100;i++){
        recv(a,buffer,1024,0);
        printf("Client : %s \n",buffer);
        send(a,"bien recu \n",12,0);
    }
    close(a);
    close(sockfd);
    return 0;
}