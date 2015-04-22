#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/types.h>
#include <unistd.h>
#include <netdb.h>

#define BUFF_SIZE 1024
#define MAX_LENGTH 1000
#define STDIN_READY 111
#define SERVER_READY 333


int wait_for_message(char *message, int server_socket_num) {
    fd_set fd_read;
    FD_ZERO(&fd_read);
    FD_SET(STDIN_FILENO, &fd_read);
    FD_SET(server_socket_num, &fd_read);
    
    int max = (server_socket_num > STDIN_FILENO) ? server_socket_num : STDIN_FILENO;
    
    printf("waiting for message\n");
    select(max+1, (fd_set*) &fd_read, (fd_set*) 0, (fd_set*) 0, NULL);
    
    if (FD_ISSET(STDIN_FILENO, &fd_read)) {
        if (read(STDIN_FILENO, message, BUFF_SIZE) < 0) {
            perror("Couldn't read the message from stdin");
        }

        return STDIN_READY;
    } else if (FD_ISSET(server_socket_num, &fd_read)) {
        return SERVER_READY;
    }
    
    return -1;
}

void watch_for_messages(int socket_num) {
    int active = 1;
    while (active) {
        char message[BUFF_SIZE];
        memset(message, 0, BUFF_SIZE);
        
        int socket_ready = wait_for_message(message, socket_num);
    
        if (socket_ready == STDIN_READY) {
            if (strlen(message) > MAX_LENGTH) {
                printf("The message is too long (max: 1000 bytes) %lu", strlen(message));
            } else {
                printf("Sending message %lu\n", strlen(message));
                send(socket_num, message, BUFF_SIZE, 0);
            }
        } else if (socket_ready == SERVER_READY) {
            int recv_result = recv(socket_num, message, BUFF_SIZE, 0);
            
            if (recv_result < 0) {
                perror("Couldn't retrieve the message");
            } else if (recv_result == 0) {
                printf("Server disconnected.");
                active = 0;
            }
            
            // handle server messages
        }
    }
}

int get_client_socket() {
    int socket_num;
    
    if ((socket_num = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Error: couldn't create socket.");
        exit(-1);
    }
    
    return socket_num;
}

void connect_to_server(int socket_num, char *host_name, char *port) {
    struct hostent *host;
    struct sockaddr_in host_addr;
    
    if ((host = gethostbyname(host_name)) == NULL) {
        perror("Error: couldn't retrieve host");
        exit(-1);
    }
    
    memcpy((char*) &host_addr.sin_addr, (char*) host->h_addr, host->h_length);
    host_addr.sin_port = htons(atoi(port));
    host_addr.sin_family = AF_INET;
    
    if (connect(socket_num, (struct sockaddr*) &host_addr, sizeof(host_addr)) < 0) {
        perror("Couldn't connect to server");
        exit(-1);
    }
}



int main(int argc, char **argv) {
    int socket_num = get_client_socket();
    connect_to_server(socket_num, argv[1], argv[2]);
    watch_for_messages(socket_num);
}