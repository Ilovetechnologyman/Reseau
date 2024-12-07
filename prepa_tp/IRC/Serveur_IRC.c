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

#define MAX_CLIENTS 30
#define BUFFER_SIZE 2048
#define PORT 8000

typedef struct {
    int socket;
    char pseudo[32];
} client_t;

int main() {
    int master_socket, new_socket;
    int max_sd, activity, i, valread, sd;
    struct sockaddr_in address;
    char buffer[BUFFER_SIZE];
    fd_set readfds;
    int addrlen = sizeof(address);
    client_t clients[MAX_CLIENTS];
    
    // Initialize client sockets array
    for (i = 0; i < MAX_CLIENTS; i++) {
        clients[i].socket = 0;
        clients[i].pseudo[0] = '\0';
    }
    
    // Create master socket
    if ((master_socket = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    
    // Set socket options
    int opt = 1;
    if (setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    
    // Setup server address structure
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr("127.0.0.1");
    address.sin_port = htons(PORT);
    
    // Bind
    if (bind(master_socket, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    
    // Listen
    if (listen(master_socket, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    
    printf("Server listening on port %d\n", PORT);
    
    while(1) {
        // Clear socket set
        FD_ZERO(&readfds);
        
        // Add master socket to set
        FD_SET(master_socket, &readfds);
        max_sd = master_socket;
        
        // Add child sockets to set
        for (i = 0; i < MAX_CLIENTS; i++) {
            sd = clients[i].socket;
            if(sd > 0)
                FD_SET(sd, &readfds);
            if(sd > max_sd)
                max_sd = sd;
        }
        
        // Wait for activity
        activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);
        if (activity < 0) {
            printf("select error");
        }
        
        // New connection
        if (FD_ISSET(master_socket, &readfds)) {
            if ((new_socket = accept(master_socket, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
                perror("accept");
                exit(EXIT_FAILURE);
            }
            
            char pseudo[32];
            recv(new_socket, pseudo, 32, 0);
            
            // Vérifier si le pseudo existe déjà
            int pseudo_exists = 0;
            for (i = 0; i < MAX_CLIENTS; i++) {
                if (clients[i].socket != 0 && strcmp(clients[i].pseudo, pseudo) == 0) {
                    pseudo_exists = 1;
                    break;
                }
            }
            
            if (pseudo_exists) {
                send(new_socket, "Pseudo already in use\n", 21, 0);
                close(new_socket);
            } else {
                for (i = 0; i < MAX_CLIENTS; i++) {
                    if (clients[i].socket == 0) {
                        clients[i].socket = new_socket;
                        strncpy(clients[i].pseudo, pseudo, 31);
                        clients[i].pseudo[31] = '\0';
                        printf("New connection from %s, socket fd is %d\n", pseudo, new_socket);
                        break;
                    }
                }
            }
        }
        
        // Check for client messages
        for (i = 0; i < MAX_CLIENTS; i++) {
            sd = clients[i].socket;
            
            if (FD_ISSET(sd, &readfds)) {
                if ((valread = read(sd, buffer, BUFFER_SIZE)) == 0) {
                    // Client disconnected
                    close(sd);
                    clients[i].socket = 0;
                }
                else {
                    buffer[valread] = '\0';
                    // Réserver de l'espace pour le format "[pseudo]: " et le message
                    char formatted_message[BUFFER_SIZE];
                    size_t prefix_len = strlen(clients[i].pseudo) + 2; // +2 pour ": "
                    
                    // S'assurer que le message ne dépasse pas la taille du buffer
                    if (valread + prefix_len >= BUFFER_SIZE) {
                        valread = BUFFER_SIZE - prefix_len - 1;
                        buffer[valread] = '\0';
                    }
                    
                    // Formater le message de manière sûre
                    int written = snprintf(formatted_message, BUFFER_SIZE, "%s: ", clients[i].pseudo);
                    strncat(formatted_message, buffer, BUFFER_SIZE - written - 1);
                    
                    for (int j = 0; j < MAX_CLIENTS; j++) {
                        if (clients[j].socket != 0 && clients[j].socket != sd) {
                            send(clients[j].socket, formatted_message, strlen(formatted_message), 0);
                        }
                    }
                }
            }
        }
    }
    
    return 0;
}