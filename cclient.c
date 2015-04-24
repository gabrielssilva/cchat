#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <netdb.h>
#include "packets.h"

#define STDIN_READY 111
#define SERVER_READY 333

struct normal_header build_header(int flag) {
    static uint32_t seq_number = 0;
    struct normal_header header;
    
    header.seq_number = htonl(seq_number);
    header.flag = flag;
    
    seq_number++;
    return header;
}

void send_message(int socket_num, char *src_handle,
                  char *dst_handle, char *message) {
    struct normal_header header = build_header(CLIENT_MESSAGE);
    uint8_t src_handle_len = strlen(src_handle);
    uint8_t dst_handle_len = strlen(dst_handle);
    
    char packet[HEADER_LENGTH + HANDLE_LENGTH + dst_handle_len
                + HANDLE_LENGTH + src_handle_len + MESSAGE_LENGTH];
    
    memcpy(packet, &header, HEADER_LENGTH);
    memcpy(packet+HEADER_LENGTH, &dst_handle_len, HANDLE_LENGTH);
    memcpy(packet+HEADER_LENGTH+HANDLE_LENGTH, dst_handle, dst_handle_len);
    memcpy(packet+HEADER_LENGTH+HANDLE_LENGTH+dst_handle_len,
           &src_handle_len, HANDLE_LENGTH);
    memcpy(packet+HEADER_LENGTH+HANDLE_LENGTH+dst_handle_len+HANDLE_LENGTH,
           src_handle, src_handle_len);
    memcpy(packet+HEADER_LENGTH+HANDLE_LENGTH+dst_handle_len+HANDLE_LENGTH+src_handle_len,
           message, MESSAGE_LENGTH);
    
    send(socket_num, packet, sizeof(packet), 0);
}

void send_broadcast(int socket_num, char *src_handle, char *message) {
    struct normal_header header = build_header(CLIENT_BROADCAST);
    uint8_t src_handle_len = strlen(src_handle);
    
    char packet[HEADER_LENGTH + HANDLE_LENGTH + src_handle_len + MESSAGE_LENGTH];
    memcpy(packet, &header, HEADER_LENGTH);
    memcpy(packet+HEADER_LENGTH, &src_handle_len, HANDLE_LENGTH);
    memcpy(packet+HEADER_LENGTH+HANDLE_LENGTH, src_handle, src_handle_len);
    memcpy(packet+HEADER_LENGTH+HANDLE_LENGTH+src_handle_len,
           message, MESSAGE_LENGTH);
    
    send(socket_num, packet, sizeof(packet), 0);
}

int handle_user_input(int socket_num, char *src_handle) {
    // If there's more time, doa further validation on the input
    char message[BUFF_SIZE];
    memset(message, 0, BUFF_SIZE);

    if (read(STDIN_FILENO, message, BUFF_SIZE) < 0) {
        perror("Couldn't read from stdin");
        return -1;
    }
    
    if (strcmp(message, "\n") == 0) {
        return -1;
    }
    
    char *command = strtok(message, " ");
    
    if (strcasecmp(command, "%M") == 0) {
        char *dst_handle = strtok(NULL, " ");
        char *msg = strtok(NULL, "\n");
        
        if (strcmp(dst_handle, "\n") == 0) {
            return -1;
        }
        char empty_msg[] = "\n";
        if (msg == NULL) {
            msg = empty_msg;
        }
        if (strlen(msg) > MESSAGE_LENGTH) {
            printf("Error, message too long, message length is: %lu", strlen(msg));
        } else {
            send_message(socket_num, src_handle, dst_handle, msg);
        }
    } else if (strcasecmp(command, "%B") == 0) {
        // Send empty message!
        char *msg = strtok(NULL, "");
        send_broadcast(socket_num, src_handle, msg);
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

void watch_for_messages(int socket_num, char *handle) {
    int active = 1;
    while (active) {
        printf("$: ");
        fflush(stdout);
        
        int socket_ready = wait_for_message(socket_num);
        if (socket_ready == STDIN_READY) {
            if (handle_user_input(socket_num, handle) < 0) {
                printf("Invalid command\n");
            }
        } else if (socket_ready == SERVER_READY) {
            char message[BUFF_SIZE];
            memset(message, 0, BUFF_SIZE);
            
            int recv_result = recv(socket_num, message, BUFF_SIZE, 0);
            
            if (recv_result < 0) {
                perror("Couldn't retrieve the message");
            } else if (recv_result == 0) {
                printf("\nServer disconnected.\n");
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

void validate_handle(int socket_num, char *handle) {
    struct normal_header header = build_header(CLIENT_INITIAL_PACKET);
    
    uint8_t handle_length = strlen(handle);
    char packet[HEADER_LENGTH + HANDLE_LENGTH + handle_length];
    memcpy(packet, &header, HEADER_LENGTH);
    memcpy(packet+HEADER_LENGTH, &handle_length, HANDLE_LENGTH);
    memcpy(packet+HEADER_LENGTH+HANDLE_LENGTH, handle, handle_length);
    send(socket_num, packet, sizeof(packet), 0);
    
    char response[HEADER_LENGTH];
    if (recv(socket_num, response, HEADER_LENGTH, 0) < 0) {
        perror("Couldn't get server response");
        exit(-1);
    }
    
    struct normal_header *rcv_header = (struct normal_header*) response;
    if (rcv_header->flag == SERVER_REJECT_HANDLE) {
        printf("Handle already in use: %s\n", handle);
        exit(-1);
    }
}

void connect_to_server(int socket_num, char *handle, char *host_name, char *port) {
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
    
    validate_handle(socket_num, handle);
}



int main(int argc, char **argv) {
    if (argc != 4) {
        printf("\nUsage: cclient <handle> <server name/address> <port number>\n\n");
        exit(-1);
    }
    
    if (strlen(argv[1]) > HANDLE_MAX_LENGTH) {
        printf("Error, handle too long, handle length is: %lu\n", strlen(argv[1]));
        exit(-1);
    }
    
    int socket_num = get_client_socket();
    connect_to_server(socket_num, argv[1], argv[2], argv[3]);
    watch_for_messages(socket_num, argv[1]);
}