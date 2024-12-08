#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <pthread.h>
#include <netdb.h>
#include <time.h>

#define BUFFER_SIZE 2048
#define MAX_PSEUDO_LENGTH 32
#define NTP_TIMESTAMP_DELTA 2208988800ull // Seconds between 1900 and 1970
#define NTP_SERVER "pool.ntp.org"
#define NTP_PORT 123

typedef struct {
    uint32_t integer;
    uint32_t fractional;
} ntp_timestamp_t;

typedef struct {
    uint8_t li_vn_mode;
    uint8_t stratum;
    uint8_t poll;
    uint8_t precision;
    uint32_t root_delay;
    uint32_t root_dispersion;
    uint32_t ref_id;
    ntp_timestamp_t reference_ts;
    ntp_timestamp_t originate_ts;
    ntp_timestamp_t receive_ts;
    ntp_timestamp_t transmit_ts;
} ntp_packet;

time_t get_ntp_time() {
    int sockfd;
    struct sockaddr_in serv_addr;
    ntp_packet packet = { 0 };
    
    packet.li_vn_mode = 0x1b; // Version 3, Mode 3 (client)
    
    sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sockfd < 0) return time(NULL); // Fallback to system time
    
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(NTP_PORT);
    
    struct hostent *server = gethostbyname(NTP_SERVER);
    if (!server) {
        close(sockfd);
        return time(NULL);
    }
    
    memcpy(&serv_addr.sin_addr.s_addr, server->h_addr_list[0], server->h_length);
    
    if (sendto(sockfd, &packet, sizeof(ntp_packet), 0,
               (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        close(sockfd);
        return time(NULL);
    }
    
    struct timeval timeout = {1, 0}; // 1 second timeout
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    
    if (recvfrom(sockfd, &packet, sizeof(ntp_packet), 0, NULL, NULL) < 0) {
        close(sockfd);
        return time(NULL);
    }
    
    close(sockfd);
    
    packet.transmit_ts.integer = ntohl(packet.transmit_ts.integer);
    return packet.transmit_ts.integer - NTP_TIMESTAMP_DELTA;
}

void print_time_message(const char *message) {
    time_t ntp_time = get_ntp_time();
    char time_str[26];
    strftime(time_str, sizeof(time_str), "%H:%M:%S", localtime(&ntp_time));
    printf("[%s] %s", time_str, message);
}

void stop(char * message){
    perror(message);
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <IP> <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char pseudo[MAX_PSEUDO_LENGTH];
    printf("Entrez votre pseudo : ");
    if (fgets(pseudo, MAX_PSEUDO_LENGTH, stdin) == NULL) {
        stop("Erreur de lecture du pseudo");
    }
    // Supprimer le retour à la ligne
    pseudo[strcspn(pseudo, "\n")] = 0;

    char *server_ip = argv[1];
    int server_port = atoi(argv[2]);
    int clisockfd;
    clisockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(clisockfd < 0) stop("socket()");
    //deff du serveur
    struct sockaddr_in serv_addr;
    memset((char *) &serv_addr, 0,sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(server_port);
    serv_addr.sin_addr.s_addr = inet_addr(server_ip);
    if (connect(clisockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) stop("connect()");

    // Envoi du pseudo
    if (send(clisockfd, pseudo, strlen(pseudo), 0) < 0) {
        stop("Erreur d'envoi du pseudo");
    }

    char buffer[BUFFER_SIZE];
    ssize_t recv_size;

    // Réception du message de bienvenue
    recv_size = recv(clisockfd, buffer, sizeof(buffer)-1, 0);
    if (recv_size > 0) {
        buffer[recv_size] = '\0';
        printf("%s", buffer);
    }

    // Boucle principale de chat
    while (1) {
        memset(buffer, 0, sizeof(buffer));
        if (fgets(buffer, BUFFER_SIZE, stdin) != NULL) {
            send(clisockfd, buffer, strlen(buffer), 0);
        }

        // Vérifier s'il y a des messages à recevoir
        fd_set readfds;
        struct timeval tv = {0, 100000}; // 100ms timeout
        FD_ZERO(&readfds);
        FD_SET(clisockfd, &readfds);
        
        if (select(clisockfd + 1, &readfds, NULL, NULL, &tv) > 0) {
            memset(buffer, 0, sizeof(buffer));
            recv_size = recv(clisockfd, buffer, sizeof(buffer)-1, 0);
            if (recv_size > 0) {
                buffer[recv_size] = '\0';
                print_time_message(buffer);
            }
        }
    }

    close(clisockfd);
    return 0;
}