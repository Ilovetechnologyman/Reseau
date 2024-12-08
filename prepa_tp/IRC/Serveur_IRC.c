#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <sys/select.h>
#include <time.h>

#define BUFFER_SIZE 2048
#define PORT 8000
#define MAX_CLIENTS 10
#define MAX_PSEUDO_LENGTH 32

typedef struct {
    int socket;
    char pseudo[MAX_PSEUDO_LENGTH];
} Client;

Client clients[MAX_CLIENTS];  // Remplace client_sockets[]
int num_clients = 0;  


void stop(char * message){
    perror(message);
    exit(EXIT_FAILURE);
}

int find_client_by_pseudo(const char* pseudo) {
    for (int i = 0; i < num_clients; i++) {
        if (strcmp(clients[i].pseudo, pseudo) == 0) {
            return i;
        }
    }
    return -1;
}

void broadcast_message(const char *message, size_t message_length, int sender_socket) {
    time_t now = time(NULL);
    char time_str[26];
    strftime(time_str, sizeof(time_str), "[%H:%M:%S] ", localtime(&now));
    
    char formatted_message[BUFFER_SIZE];
    snprintf(formatted_message, sizeof(formatted_message), "%s%s", time_str, message);
    
    for (int i = 0; i < num_clients; ++i) {
        if (clients[i].socket != -1 && clients[i].socket != sender_socket) {
            send(clients[i].socket, formatted_message, strlen(formatted_message), 0);
        }
    }
}

int main() {
    int sockfd;
    struct sockaddr_in serv_addr;
    sockfd = socket(AF_INET, SOCK_STREAM, 0); // IPV4 TCP 
    if(sockfd < 0) stop("socket()");
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    serv_addr.sin_port = htons(8000);
    if(bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) stop("bind()");
    if(listen(sockfd, MAX_CLIENTS) < 0) stop("listen()");
    srand(time(NULL));
    
    // Initialize clients array
    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].socket = -1;
        memset(clients[i].pseudo, 0, MAX_PSEUDO_LENGTH);
    }
    
    fd_set readfds;
    int max_sd;
    
    while (1) {
        FD_ZERO(&readfds);
        FD_SET(sockfd, &readfds);
        max_sd = sockfd;
        
        // Ajouter les sockets clients au set
        for (int i = 0; i < num_clients; i++) {
            int sd = clients[i].socket;
            if (sd > 0) {
                FD_SET(sd, &readfds);
                if (sd > max_sd) {
                    max_sd = sd;
                }
            }
        }
        
        // Attendre une activité sur l'un des sockets
        if (select(max_sd + 1, &readfds, NULL, NULL, NULL) < 0) {
            stop("select()");
        }
        
        // Si c'est le socket d'écoute
        if (FD_ISSET(sockfd, &readfds)) {
            struct sockaddr_in client_addr;
            socklen_t addr_len = sizeof(client_addr);
            int client_socket = accept(sockfd, (struct sockaddr *)&client_addr, &addr_len);
            
            if (client_socket >= 0) {
                // Check if server is full
                if (num_clients >= MAX_CLIENTS) {
                    const char *error_msg = "Serveur plein. Réessayez plus tard.\n";
                    send(client_socket, error_msg, strlen(error_msg), 0);
                    close(client_socket);
                    continue;
                }

                // Réception du pseudo
                char pseudo[MAX_PSEUDO_LENGTH];
                ssize_t recv_size = recv(client_socket, pseudo, MAX_PSEUDO_LENGTH-1, 0);
                if (recv_size > 0) {
                    pseudo[recv_size] = '\0';
                    
                    // Vérification du pseudo unique
                    if (find_client_by_pseudo(pseudo) != -1) {
                        const char *error_msg = "Pseudo déjà utilisé\n";
                        send(client_socket, error_msg, strlen(error_msg), 0);
                        close(client_socket);
                        continue;
                    }

                    // Ajout du nouveau client
                    clients[num_clients].socket = client_socket;
                    strncpy(clients[num_clients].pseudo, pseudo, MAX_PSEUDO_LENGTH);
                    
                    printf("Nouvelle connexion : %s (socket %d)\n", pseudo, client_socket);
                    
                    char welcome_msg[BUFFER_SIZE];
                    snprintf(welcome_msg, sizeof(welcome_msg), "Bienvenue %s sur le serveur !\n", pseudo);
                    send(client_socket, welcome_msg, strlen(welcome_msg), 0);

                    char broadcast_msg[BUFFER_SIZE];
                    snprintf(broadcast_msg, sizeof(broadcast_msg), "%s a rejoint le serveur !\n", pseudo);
                    broadcast_message(broadcast_msg, strlen(broadcast_msg), client_socket);
                    
                    num_clients++;
                }
            }
        }
        
        // Vérifier les messages des clients existants
        for (int i = 0; i < num_clients; i++) {
            int sd = clients[i].socket;
            
            if (FD_ISSET(sd, &readfds)) {
                char buffer[BUFFER_SIZE];
                ssize_t valread = recv(sd, buffer, BUFFER_SIZE, 0);
                
                if (valread <= 0) {
                    // Client déconnecté
                    char disconnect_msg[BUFFER_SIZE];
                    snprintf(disconnect_msg, sizeof(disconnect_msg), "%s a quitté le serveur.\n", clients[i].pseudo);
                    close(sd);
                    clients[i].socket = -1;
                    // Décaler les clients restants
                    for (int j = i; j < num_clients - 1; j++) {
                        clients[j] = clients[j + 1];
                    }
                    num_clients--;
                    broadcast_message(disconnect_msg, strlen(disconnect_msg), sd);
                }
                else {
                    buffer[valread] = '\0';
                    // Formater et diffuser le message
                    char formatted_msg[BUFFER_SIZE];
                    snprintf(formatted_msg, sizeof(formatted_msg), "%s: %s", clients[i].pseudo, buffer);
                    broadcast_message(formatted_msg, strlen(formatted_msg), sd);
                }
            }
        }
    }
    close(sockfd);
}