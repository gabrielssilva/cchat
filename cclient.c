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

struct normal_header build_header(uint8_t flag) {
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

void send_request(int socket_num, uint8_t flag) {
    struct normal_header header = build_header(flag);
    char packet[HEADER_LENGTH];
    memcpy(packet, &header, HEADER_LENGTH);
    
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
    
    char *cp_msg = strdup(message);
    char *command = strtok(cp_msg, " ");
    command = strtok(command, "\n");
    
    if (strcasecmp(command, "%M") == 0) {
        command = strtok(message, " ");
        char *dst_handle = strtok(NULL, " ");
        char *msg = strtok(NULL, "\n");
        dst_handle = strtok(dst_handle, "\n");
        
        if (dst_handle == NULL) {
            printf("Error, no handle given\n");
        } else {
            if (msg == NULL) {
                msg = strdup("\n");
            }
            if (strlen(msg) > MESSAGE_LENGTH-1) {
                printf("Error, message too long, message length is: %lu\n", strlen(msg)+1);
            } else {
                send_message(socket_num, src_handle, dst_handle, msg);
            }
        }
        
        printf("$: ");
        fflush(stdout);
    } else if (strcasecmp(command, "%B") == 0) {
        command = strtok(message, " ");
        char *msg = strtok(NULL, "\n");
        
        if (msg == NULL) {
            msg = strdup("\n");
        }
        
        send_broadcast(socket_num, src_handle, msg);
        printf("$: ");
        fflush(stdout);
    } else if (strcasecmp(command, "%L") == 0) {
        send_request(socket_num, CLIENT_HANDLE_REQUEST);
        
        printf("$: ");
        fflush(stdout);
    } else if (strcasecmp(command, "%E") == 0) {
        send_request(socket_num, CLIENT_EXIT_REQUEST);
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

void print_broadcast(char *packet) {
    uint8_t handle_length = *(packet + HEADER_LENGTH);
    
    char handle[handle_length+1];
    memcpy(handle, (packet + HEADER_LENGTH + HANDLE_LENGTH), handle_length);
    handle[handle_length] = '\0';
    
    char *message = (packet + HEADER_LENGTH + HANDLE_LENGTH + handle_length);
    printf("\n%s: %s\n", handle, message);
}

void print_message(char *packet) {
    uint8_t dst_handle_length = *(packet + HEADER_LENGTH);
    uint8_t src_handle_length = *(packet + HEADER_LENGTH + HANDLE_LENGTH + dst_handle_length);
    
    char src_handle[src_handle_length+1];
    memcpy(src_handle, (packet + HEADER_LENGTH + HANDLE_LENGTH
                        + dst_handle_length + HANDLE_LENGTH), src_handle_length);
    src_handle[src_handle_length] = '\0';
    
    char *message = (packet + HEADER_LENGTH + HANDLE_LENGTH + dst_handle_length
                     + HANDLE_LENGTH + src_handle_length);
    
    printf("\n%s: %s\n", src_handle, message);
}

void print_handles_list(int socket_num, char *packet) {
    uint32_t num_handles = *(packet + HEADER_LENGTH);
    printf("\n");
    
    uint8_t i=0;
    for (i=0; i<num_handles; i++) {
        char handle_packet[HANDLE_LENGTH + HANDLE_MAX_LENGTH];
        if (recv(socket_num, handle_packet, sizeof(handle_packet), 0) < 0) {
            perror("Couldn't get server response");
            exit(-1);
        }
        
        uint8_t handle_length = *(handle_packet);
        char handle[HANDLE_MAX_LENGTH];
        memset(handle, 0, HANDLE_MAX_LENGTH);
        memcpy(handle, (handle_packet + HANDLE_LENGTH), handle_length);
        printf("%s\n", handle);
    }
}

void handle_server_messages(int socket_num, char *packet) {
    struct normal_header header;
    memcpy(&header, packet, HEADER_LENGTH);
    
    if (header.flag == CLIENT_MESSAGE) {
        print_message(packet);
        printf("$: ");
        fflush(stdout);
    } else if (header.flag == CLIENT_BROADCAST) {
        print_broadcast(packet);
        printf("$: ");
        fflush(stdout);
    } else if (header.flag == SERVER_HANDLE_ERROR) {
        uint8_t handle_length = *(packet + HEADER_LENGTH);
        char handle[handle_length+1];
        memcpy(handle, (packet + HEADER_LENGTH + HANDLE_LENGTH), handle_length);
        handle[handle_length] = '\0';
        
        printf("\nClient with handle %s does not exist\n", handle);
        printf("$: ");
        fflush(stdout);
    } else if (header.flag == SERVER_HANDLE_LIST) {
        print_handles_list(socket_num, packet);
        printf("$: ");
        fflush(stdout);
    } else if (header.flag == SERVER_ACK_EXIT) {
        exit(0);
    }
}

void watch_for_messages(int socket_num, char *handle) {
    printf("$: ");
    fflush(stdout);
    
    while (1) {
        int socket_ready = wait_for_message(socket_num);
        if (socket_ready == STDIN_READY) {
            if (handle_user_input(socket_num, handle) < 0) {
                printf("Invalid command\n");
                printf("$: ");
                fflush(stdout);
            }
        } else if (socket_ready == SERVER_READY) {
            char packet[BUFF_SIZE];
            
            int recv_result = recv(socket_num, packet, BUFF_SIZE, 0);
            
            if (recv_result < 0) {
                perror("Couldn't retrieve the message");
            } else if (recv_result == 0) {
                printf("\nServer Terminated\n");
                exit(-1);
            }
            
            handle_server_messages(socket_num, packet);
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