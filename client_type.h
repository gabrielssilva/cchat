#ifndef ____client_type__
#define ____client_type__

#include <stdio.h>
#include <stdlib.h>

struct client_t {
    int fd;
    struct client_t *next;
};

struct clients_t {
    struct client_t *client;
    int num_clients;
};

void add_client(struct clients_t**, int);
void remove_client(struct clients_t**, int);
int get_client(struct clients_t*, int);
void print(struct clients_t*);

#endif