#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <netdb.h>
#include "client_type.h"
#include "packets.h"


struct normal_header build_header(uint8_t flag) {
    static uint32_t seq_number = 0;
    struct normal_header header;
    
    header.seq_number = htonl(seq_number);
    header.flag = flag;
    
    seq_number++;
    return header;
}

void send_simple_packet(int socket_num, uint8_t flag) {
    struct normal_header header = build_header(flag);
    
    char packet[HEADER_LENGTH];
    memcpy(packet, &header, HEADER_LENGTH);
    printf("Simple packet: %lu\n", sizeof(packet));
    send(socket_num, packet, sizeof(packet), 0);
}

int get_client_socket(struct clients_t *clients, char *handle, uint8_t handle_length) {
    struct client_t *temp = clients->client;
    while (temp != NULL) {
        if (strncasecmp(handle, temp->handle, handle_length) == 0) {
            return temp->fd;
        }
        temp = temp->next;
    }
    
    return -1;
}

int check_handle(int socket_num, char *packet, struct clients_t *clients) {
    uint8_t handle_length = *(packet + HEADER_LENGTH);
    char *handle = (packet + HEADER_LENGTH + HANDLE_LENGTH);
    
    if (get_client_socket(clients, handle, handle_length) < 0) {
        add_handle(&clients, socket_num, handle, handle_length);
        return SERVER_ACCEPT_HANDLE;
    } else {
        return SERVER_REJECT_HANDLE;
    }
}

void handle_broadcast_packet(char *packet, int src_socket, struct clients_t *clients) {
    struct client_t *temp = clients->client;
    uint8_t src_handle_length = *(packet + HEADER_LENGTH);
    
    while (temp != NULL) {
        if (src_socket != temp->fd) {
            send(temp->fd, packet, (HEADER_LENGTH + HANDLE_LENGTH + src_handle_length
                                    + MESSAGE_LENGTH), 0);
        }
        temp = temp->next;
    }
}

void handle_message_packet(char *packet, int src_socket, struct clients_t *clients) {
    uint8_t dst_handle_length = *(packet + HEADER_LENGTH);
    char *dst_handle = (packet + HEADER_LENGTH + HANDLE_LENGTH);
    uint8_t src_handle_length = *(packet + HEADER_LENGTH + dst_handle_length);
    
    int offset = HEADER_LENGTH + HANDLE_LENGTH + dst_handle_length;
    char *message = (packet + offset + HANDLE_LENGTH + src_handle_length);
    
    int response_flag = -1;
    int dst_sokect_num;
    if ((dst_sokect_num = get_client_socket(clients, dst_handle, dst_handle_length)) < 0) {
        response_flag = SERVER_HANDLE_ERROR;
    } else if (strlen(message) < MESSAGE_LENGTH) {
        int fwd_offset = offset + HANDLE_LENGTH + src_handle_length + MESSAGE_LENGTH;
        send(dst_sokect_num, packet, fwd_offset, 0);
        response_flag = SERVER_HANDLE_OK;
    }
    
    // Respond to the client
    struct normal_header new_header = build_header(response_flag);
    memcpy(packet, &new_header, HEADER_LENGTH);
    printf("responding to client: %d\n", offset);
    send(src_socket, packet, offset, 0);
}

void handle_client_exit(int client_socket, struct clients_t *clients) {
    // Remove client from list and acknowledge successful exit
    remove_client(&clients, client_socket);
    send_simple_packet(client_socket, SERVER_ACK_EXIT);
}

void send_handles_list(int socket_num, struct clients_t *clients) {
    struct normal_header header = build_header(SERVER_HANDLE_LIST);
    uint32_t num_clients = clients->num_clients;
    char packet[HEADER_LENGTH + sizeof(uint32_t)];
    
    // Send initial packet with Handles list information
    memcpy(packet, &header, HEADER_LENGTH);
    memcpy(packet+HEADER_LENGTH, &num_clients, sizeof(uint32_t));
    send(socket_num, packet, sizeof(packet), 0);
    
    // Send all handles (a packet for each)
    struct client_t *temp = clients->client;
    while (temp != NULL) {
        uint8_t handle_length = strlen(temp->handle);
        char handle_packet[HANDLE_LENGTH + HANDLE_MAX_LENGTH];
        
        memcpy(handle_packet, &handle_length, HANDLE_LENGTH);
        memcpy(handle_packet+HANDLE_LENGTH, temp->handle, handle_length);
        if (send(socket_num, handle_packet, sizeof(handle_packet), 0) < 0) {
            perror("send");
        }
        
        temp = temp->next;
    }
}

void handle_packet(int socket_num, struct clients_t *clients, char *packet) {
    struct normal_header header;
    memcpy(&header, packet, HEADER_LENGTH);
    
    if (header.flag == CLIENT_INITIAL_PACKET) {
        if (check_handle(socket_num, packet, clients) == SERVER_ACCEPT_HANDLE) {
            send_simple_packet(socket_num, SERVER_ACCEPT_HANDLE);
        } else {
            send_simple_packet(socket_num, SERVER_REJECT_HANDLE);
        }
    } else if (header.flag == CLIENT_BROADCAST) {
        handle_broadcast_packet(packet, socket_num, clients);
    } else if (header.flag == CLIENT_MESSAGE) {
        handle_message_packet(packet, socket_num, clients);
    } else if (header.flag == CLIENT_EXIT_REQUEST) {
        handle_client_exit(socket_num, clients);
    } else if (header.flag == CLIENT_HANDLE_REQUEST) {
        send_handles_list(socket_num, clients);
    } else {
        // Invalid Flag
    }
}

void handle_client(int client_socket_num, struct clients_t *clients) {
    char packet[BUFF_SIZE];

    int recv_result = recv(client_socket_num, packet, BUFF_SIZE, 0);
    if (recv_result < 0) {
        perror("Couldn't retrieve the message.");
    } else if (recv_result == 0) {
        printf("Client disconnected\n");
        remove_client(&clients, client_socket_num);
        close(client_socket_num);
    } else {
        handle_packet(client_socket_num, clients, packet);
    }
}

int wait_for_client(int server_socket_num, struct clients_t *clients) {
    fd_set fd_read;
    FD_ZERO(&fd_read);
    FD_SET(server_socket_num, &fd_read);
    
    int max = server_socket_num;
    int i;
    for (i=0; clients != NULL && i<clients->num_clients; i++) {
        FD_SET(get_client(clients, i), &fd_read);
        max = (get_client(clients, i) > max) ? get_client(clients, i) : max;
    }
    
    select(max+1, (fd_set*) &fd_read, (fd_set*) 0, (fd_set*) 0, NULL);
    
    if (FD_ISSET(server_socket_num, &fd_read)){
        return server_socket_num;
    }

    for (i=0; clients != NULL && i<clients->num_clients; i++) {
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
            handle_client(socket_ready, clients);
        }
    }
}


void setup_server(int socket_num, int port_number) {
    struct sockaddr_in local_addr;
    socklen_t local_len = sizeof(local_addr);
    
    local_addr.sin_family = AF_INET;
    local_addr.sin_addr.s_addr = INADDR_ANY;
    local_addr.sin_port = htons(port_number);
    
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
    int port_number = 0;
    
    if (argc > 2) {
        printf("\nUsage: server <optional: port number>\n\n");
        exit(-1);
    } else if (argc == 2) {
        port_number = atoi(argv[1]);
    }
    
    int socket_num = get_server_socket();
    setup_server(socket_num, port_number);
    
    watch_for_clients(socket_num);
    
    close(socket_num);
}