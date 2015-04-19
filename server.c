#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <unistd.h>

#define BUFF_SIZE 1024

void receive_messages(int client_socket_num) {
    int active = 1;
    while (active) {
        fd_set fd_read;
        FD_ZERO(&fd_read);
        FD_SET(client_socket_num, &fd_read);
        
        select(client_socket_num+1, (fd_set*) &fd_read, (fd_set*) 0, (fd_set*) 0, NULL);
        // Possible bug. Maybe use FD_ISSET?

        char *message = malloc(BUFF_SIZE);

        if (recv(client_socket_num, message, BUFF_SIZE, 0) <= 0) {
            free(message);
            message = NULL;
            
            printf("Client disconnected\n");
            active = 0;
        } else {
            printf("%s\n", message);
        }
    }
    
    close(client_socket_num);
}

int wait_for_clients(int socket_num) {
    int client_socket_num;
    
    if (listen(socket_num, 5) < 0) {
        perror("Error: couldn't listen for connections.");
        exit(-1);
    }
    
    if ((client_socket_num = accept(socket_num, (struct sockaddr*)0, (socklen_t *) 0)) < 0) {
        perror("Error: couldn't accept client connection.");
        exit(-1);
    }

    return client_socket_num;
}


void setup_server(int socket_num) {
    struct sockaddr_in local_addr;
    socklen_t local_len = sizeof(local_addr);
    
    local_addr.sin_family = AF_INET;
    local_addr.sin_addr.s_addr = INADDR_ANY;
    local_addr.sin_port = htons(0);
    
    if (bind(socket_num, (struct sockaddr*) &local_addr, sizeof(local_addr)) < 0) {
        perror("Error: couldn't setup server port.");
        exit(-1);
    }
    
    if (getsockname(socket_num, (struct sockaddr*) &local_addr, &local_len) < 0) {
        perror("Error: couldn't retrieve server name.");
        exit(-1);
    }
    
    printf("Server is using port %d \n", ntohs(local_addr.sin_port));
}

int get_server_socket() {
    int socket_num;
    
    if((socket_num = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Error: couldn't create socket.");
        exit(-1);
    }
    
    return socket_num;
}

int main(int argc, char **argv) {
    int socket_num = get_server_socket();
    setup_server(socket_num);

    int client_socket_num = wait_for_clients(socket_num);
    receive_messages(client_socket_num);
    
    close(socket_num);
}