#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/select.h>
#include <stdlib.h>  
#include <netinet/in.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h> // Add this include for signal handling

#define PORT 8000
#define TRUE 1
#define BUFFER_SIZE 1025


#ifndef TEMP_FAILURE_RETRY
#define TEMP_FAILURE_RETRY(expression) \
    ({                                 \
        long int __result;             \
        do {                           \
            __result = (long int)(expression); \
        } while (__result == -1L && errno == EINTR); \
        __result;                      \
    })
#endif


int input_timeout(int filedes, unsigned int seconds){
    fd_set set;
    struct timeval timeout;
    FD_ZERO(&set);
    FD_SET(filedes, &set);
    timeout.tv_sec = seconds;
    timeout.tv_usec = 0;
    return TEMP_FAILURE_RETRY(select(FD_SETSIZE, &set, NULL, NULL, &timeout));
}

int master_socket; // Move this declaration to global scope

void handle_signal(int signal) {
    close(master_socket);
    printf("Socket closed and resources released.\n");
    exit(EXIT_SUCCESS);
}
void initialize_server(int *client_socket, int max_clients, struct sockaddr_in *address, int *addrlen) {
    int opt = 1;
    for (int i = 0; i < max_clients; i++) {
        client_socket[i] = 0;
    }
    if ((master_socket = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    if (setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    address->sin_family = AF_INET;
    address->sin_addr.s_addr = INADDR_ANY;
    address->sin_port = htons(PORT);
    if (bind(master_socket, (struct sockaddr *)address, sizeof(*address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(master_socket, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    *addrlen = sizeof(*address);
    puts("Waiting for connections ...");
}

void handle_new_connection(int *client_socket, int max_clients, struct sockaddr_in *address, int addrlen, char *message) {
    int new_socket;
    if((new_socket = accept(master_socket, (struct sockaddr *)address, (socklen_t*)&addrlen))<0){
        perror("accept");
        exit(EXIT_FAILURE);
    }
    printf("New connection, socket fd is %d, ip is : %s, port : %d\n", new_socket, inet_ntoa(address->sin_addr), ntohs(address->sin_port));
    
    if (send(new_socket, message, strlen(message), 0) < 0) {
        perror("send");
        close(new_socket);
        return;
    }
    puts("Welcome message sent successfully");
    int activity;
    for(int i = 0; i < max_clients; i++){
        if(client_socket[i] == 0){
            client_socket[i] = new_socket;
            printf("Adding to list of sockets as %d\n", i);
            break;
        }
    }
}

void handle_client_activity(int *client_socket, int max_clients, fd_set *readfds, struct sockaddr_in *address, int addrlen) {
    int sd, valread;
    char buffer[1025];
    for(int i = 0; i < max_clients; i++){
        sd = client_socket[i];
        if(FD_ISSET(sd, readfds)){
    if((valread = read(sd, buffer, 1024)) == 0){
        printf("Host disconnected, ip %s, port %d\n", inet_ntoa(address->sin_addr), ntohs(address->sin_port));
        close(sd);
        client_socket[i] = 0;
    } else if (valread < 0) {
        perror("read error");
        close(sd);
                if (send(sd, buffer, strlen(buffer), 0) < 0) {
                    perror("send");
                    close(sd);
                    client_socket[i] = 0;
                }
    } else {
        buffer[valread] = '\0';
        send(sd, buffer, strlen(buffer), 0);
    }
        }
    }
}
int main(int argc , char*argv[]){
    int client_socket[30], max_clients = 30, addrlen;
    struct sockaddr_in address;
    char *message = "ECHO Daemon v1.0 \r\n";
    fd_set readfds;

    // Register signal handler for graceful shutdown
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    initialize_server(client_socket, max_clients, &address, &addrlen);

    while(TRUE){
        int activity;
        FD_ZERO(&readfds);
        FD_SET(master_socket, &readfds);
        int max_sd = master_socket;
        for(int i = 0; i < max_clients; i++){
            int sd = client_socket[i];
            if(sd > 0){
                FD_SET(sd, &readfds);
            }
        if (activity == 0) {
            continue; // Timeout occurred, continue to the next iteration
        }
        if((activity < 0) && (errno != EINTR)){
                max_sd = sd;
            }
        }
        int activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);
        if((activity < 0) && (errno != EINTR)){
            printf("select error");
        }
        if(FD_ISSET(master_socket, &readfds)){
            handle_new_connection(client_socket, max_clients, &address, addrlen, message);
        }
        handle_client_activity(client_socket, max_clients, &readfds, &address, addrlen);
    }
    return 0;
}
