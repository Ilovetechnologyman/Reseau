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

#define BUFFER_SIZE 2048
#define NTP_TIMESTAMP_DELTA 2208988800ull
#define NTP_SERVER "pool.ntp.org"

// Structure pour le paquet NTP
typedef struct {
    uint8_t li_vn_mode;
    uint8_t stratum;
    uint8_t poll;
    uint8_t precision;
    uint32_t root_delay;
    uint32_t root_dispersion;
    uint32_t ref_id;
    uint32_t ref_ts_sec;
    uint32_t ref_ts_frac;
    uint32_t orig_ts_sec;
    uint32_t orig_ts_frac;
    uint32_t recv_ts_sec;
    uint32_t recv_ts_frac;
    uint32_t trans_ts_sec;
    uint32_t trans_ts_frac;
} ntp_packet;

// Fonction pour obtenir l'heure NTP
time_t get_ntp_time() {
    int sockfd;
    struct hostent *server;
    struct sockaddr_in serv_addr;
    ntp_packet packet = { 0 };
    
    packet.li_vn_mode = 0x1b; // Version 3, Mode 3 (client)
    
    sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    server = gethostbyname(NTP_SERVER);
    
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    memcpy(&serv_addr.sin_addr.s_addr, server->h_addr_list[0], server->h_length);
    serv_addr.sin_port = htons(123);
    
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        return time(NULL); // Fallback to system time
    }
    
    if (send(sockfd, &packet, sizeof(ntp_packet), 0) < 0) {
        close(sockfd);
        return time(NULL);
    }
    
    if (recv(sockfd, &packet, sizeof(ntp_packet), 0) < 0) {
        close(sockfd);
        return time(NULL);
    }
    
    close(sockfd);
    return ntohl(packet.trans_ts_sec) - NTP_TIMESTAMP_DELTA;
}

void *receive_messages(void *socket_desc) {
    int sock = *(int*)socket_desc;
    char buffer[BUFFER_SIZE];
    int read_size;
    
    while((read_size = recv(sock, buffer, BUFFER_SIZE, 0)) > 0) {
        buffer[read_size] = '\0';
        time_t ntp_time = get_ntp_time();
        struct tm *time_info = localtime(&ntp_time);
        printf("[%02d:%02d:%02d] %s", 
               time_info->tm_hour,
               time_info->tm_min,
               time_info->tm_sec,
               buffer);
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    if(argc != 3) {
        printf("Usage: %s <IP> <port>\n", argv[0]);
        return 1;
    }

    int sock;
    struct sockaddr_in server;
    char pseudo[32];
    char message[BUFFER_SIZE];
    pthread_t thread_id;

    // Create socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        printf("Could not create socket");
        return 1;
    }

    server.sin_addr.s_addr = inet_addr(argv[1]);
    server.sin_family = AF_INET;
    server.sin_port = htons(atoi(argv[2]));

    // Connect to server
    if (connect(sock, (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("Connect failed");
        return 1;
    }

    printf("Enter your pseudo: ");
    fgets(pseudo, 32, stdin);
    pseudo[strcspn(pseudo, "\n")] = 0;

    // Send pseudo to server
    send(sock, pseudo, strlen(pseudo), 0);

    // Create thread for receiving messages
    if(pthread_create(&thread_id, NULL, receive_messages, (void*)&sock) < 0) {
        perror("Could not create thread");
        return 1;
    }

    // Main loop for sending messages
    while(1) {
        fgets(message, BUFFER_SIZE, stdin);
        if(send(sock, message, strlen(message), 0) < 0) {
            puts("Send failed");
            return 1;
        }
    }

    return 0;
}
