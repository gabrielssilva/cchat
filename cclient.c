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


int handle_user_input(int socket_num) {
    // If there's more time, doa further validation on the input
    char message[BUFF_SIZE];
    memset(message, 0, BUFF_SIZE);

    if (read(STDIN_FILENO, message, BUFF_SIZE) < 0) {
        perror("Couldn't read from stdin");
    }
    
    char *command = strtok(message, " ");
    char *handle = strtok(NULL, " ");
    char *msg = strtok(NULL, "\n");
    
    if (strcasecmp(command, "%M") == 0) {
        char empty_msg[] = "\n";
        
        
        if (strcmp(handle, "\n") == 0) {
            return -1;
        }
        
        if (msg == NULL) {
            msg = empty_msg;
        }
        
        if (strlen(msg) > MAX_LENGTH) {
            printf("The message is too long (max: 1000 bytes) %lu", strlen(msg));
        } else {
            printf("Sending message %lu\n", strlen(msg));
            send(socket_num, msg, BUFF_SIZE, 0);
        }
    } else if (strcasecmp(strtok(command, "\n"), "%B") == 0) {
        char *msg = strtok(NULL, "");
        printf("broadcast\n");
    } else if (strcasecmp(strtok(command, "\n"), "%L") == 0) {
        printf("handles\n");
    } else if (strcasecmp(strtok(command, "\n"), "%E") == 0) {
        printf("exit\n");
    } else {
        return -1;
    }
    
    return 0;
}

int wait_for_message(int server_socket_num) {
    fd_set fd_read;
    FD_ZERO(&fd_read);
    FD_SET(STDIN_FILENO, &fd_read);
    FD_SET(server_socket_num, &fd_read);
    
    int max = (server_socket_num > STDIN_FILENO) ? server_socket_num : STDIN_FILENO;
    select(max+1, (fd_set*) &fd_read, (fd_set*) 0, (fd_set*) 0, NULL);
    
    if (FD_ISSET(STDIN_FILENO, &fd_read)) {
        return STDIN_READY;
    } else if (FD_ISSET(server_socket_num, &fd_read)) {
        return SERVER_READY;
    }
    
    return -1;
}

void watch_for_messages(int socket_num) {
    int active = 1;
    while (active) {
        printf("$: ");
        fflush(stdout);
        
        int socket_ready = wait_for_message(socket_num);
        if (socket_ready == STDIN_READY) {
            if (handle_user_input(socket_num) < 0) {
                printf("Invalid command\n");
            }
        } else if (socket_ready == SERVER_READY) {
            char message[BUFF_SIZE];
            memset(message, 0, BUFF_SIZE);
            
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
    if (argc != 3) {
        printf("\nUsage: cclient <server name/address> <port number>\n\n");
        exit(-1);
    }
    
    int socket_num = get_client_socket();
    connect_to_server(socket_num, argv[1], argv[2]);
    watch_for_messages(socket_num);
}