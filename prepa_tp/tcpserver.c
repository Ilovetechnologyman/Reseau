#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <strings.h>
#include <errno.h>
int server_socket = -1;

int main(){
    int sockfd = socket(AF_INET, SOCK_STREAM, 0); // socket type TCP IPV4
    if (sockfd < 0) {
        printf("Error creating socket\n");
        return -1;
    }   
    struct sockaddr_in server_addr;
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(1234); // port 1234
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); // 127.0.0.1
    if (bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        printf("Error binding socket\n");
        close(sockfd);
        return -1;
    }
    if (listen(sockfd, 5) < 0) { // 5 clients
        printf("Error listening on socket\n");
        close(sockfd);
        return -1;
    }
    int client_socket;
    while ((client_socket = accept(sockfd, NULL, NULL)) >= 0) {
        if (errno == EINTR) {
            continue;
        } else if (client_socket < 0) {
            printf("Error accepting client\n");
            close(sockfd);
            return -1;
        }
        printf("New client connected\n");
        
        char buffer[1024];
        while (1) {
            bzero(buffer, sizeof(buffer));
            ssize_t received = recv(client_socket, buffer, sizeof(buffer), 0);
            if (received <= 0) {
                printf("Client disconnected\n");
                break;
            }
            
            printf("Received: %s\n", buffer);
            
            if (send(client_socket, buffer, received, 0) < 0) {
                printf("Error sending data\n");
                break;
            }
        }
        close(client_socket);
    }
    close(sockfd);
}