#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <unistd.h>
#include "client_type.h"

#pragma pack(1)

#define BUFF_SIZE 1024


void handle_client(int client_socket_num) {
    char *message = malloc(BUFF_SIZE);

    int recv_result = recv(client_socket_num, message, BUFF_SIZE, 0);
    if (recv_result < 0) {
        free(message);
        message = NULL;
        
        perror("Couldn't retrieve the message.");
    } else if (recv_result == 0) {
        free(message);
        message = NULL;
            
        printf("Client disconnected\n");
        //close(client_socket_num);
    } else {
        printf("%s\n", message);
    }
}

int wait_for_client(int server_socket_num, struct clients_t *clients) {
    fd_set fd_read;
    FD_ZERO(&fd_read);
    FD_SET(server_socket_num, &fd_read);
    
    int max = server_socket_num;
    for (int i=0; clients != NULL && i<clients->num_clients; i++) {
        FD_SET(get_client(clients, i), &fd_read);
        max = (get_client(clients, i) > max) ? get_client(clients, i) : max;
    }
    
    select(max+1, (fd_set*) &fd_read, (fd_set*) 0, (fd_set*) 0, NULL);
    
    if (FD_ISSET(server_socket_num, &fd_read)){
        return server_socket_num;
    }

    for (int i=0; clients != NULL && i<clients->num_clients; i++) {
        int client_socket = get_client(clients, i);
        if (FD_ISSET(client_socket, &fd_read)) {
            return client_socket;
        }
    }
    
    return -1;
}

void watch_for_clients(int socket_num) {
    struct clients_t *clients = NULL;
    
    if (listen(socket_num, 5) < 0) {
        perror("Couldn't listen for connections.");
        exit(-1);
    }
    
    while (1) {
        int socket_ready = wait_for_client(socket_num, clients);
        if (socket_ready == socket_num) {
            int client_socket_num;
            if ((client_socket_num = accept(socket_num, (struct sockaddr*) 0, (socklen_t*) 0)) < 0) {
                perror("Couldn't accept client connection.");
                exit(-1);
            }
            
            add_client(&clients, client_socket_num);
            printf("client connected\n");
        } else {
            handle_client(socket_ready);
        }
    }
}


void setup_server(int socket_num) {
    struct sockaddr_in local_addr;
    socklen_t local_len = sizeof(local_addr);
    
    local_addr.sin_family = AF_INET;
    local_addr.sin_addr.s_addr = INADDR_ANY;
    local_addr.sin_port = htons(0);
    
    if (bind(socket_num, (struct sockaddr*) &local_addr, sizeof(local_addr)) < 0) {
        perror("Couldn't setup server port.");
        exit(-1);
    }
    
    if (getsockname(socket_num, (struct sockaddr*) &local_addr, &local_len) < 0) {
        perror("Couldn't retrieve server name.");
        exit(-1);
    }
    
    printf("Server is using port %d \n", ntohs(local_addr.sin_port));
}

int get_server_socket() {
    int socket_num;
    
    if((socket_num = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Couldn't create socket.");
        exit(-1);
    }
    
    return socket_num;
}

int main(int argc, char **argv) {
    int socket_num = get_server_socket();
    setup_server(socket_num);
    
    watch_for_clients(socket_num);
    
    close(socket_num);
}