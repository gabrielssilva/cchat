#ifndef ____client_type__
#define ____client_type__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#pragma pack(1)

#define HANDLE_MAX_LENGTH 100

struct client_t {
    int fd;
    char handle[HANDLE_MAX_LENGTH];
    struct client_t *next;
};

struct clients_t {
    struct client_t *client;
    int num_clients;
};


void add_client(struct clients_t**, int);
void add_handle(struct clients_t**, int, char*, uint8_t);
void remove_client(struct clients_t**, int);
int get_client(struct clients_t*, int);
void print(struct clients_t*);

#endif