#include "client_type.h"



void check_list(struct clients_t **clients) {
    if (*clients == NULL) {
        *clients = (struct clients_t*) malloc(sizeof(struct clients_t));
        (*clients)->num_clients = 0;
        (*clients)->client = NULL;
    }
}

void add_client(struct clients_t **clients, int fd) {
    check_list(clients);
    
    struct client_t *new_client = (struct client_t*) malloc(sizeof(struct client_t));
    new_client->fd = fd;
    new_client->next = NULL;
    
    if ((*clients)->client == NULL) {
        (*clients)->client = new_client;
    } else {
        struct client_t *temp = (*clients)->client;
        while (temp->next != NULL) {
            temp = temp->next;
        }
        temp->next = new_client;
    }
    
    (*clients)->num_clients++;
}

int get_client(struct clients_t *clients, int index) {
    if (clients == NULL) {
        return -1;
    } else {
        struct client_t *temp = clients->client;
        while (index > 0) {
            temp = temp->next;
            index = index-1;
        }
        return temp->fd;
    }
}