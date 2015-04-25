#ifndef _packets_h
#define _packets_h

#include <stdint.h>

#define BUFF_SIZE 1440
#define MESSAGE_LENGTH 1000
#define HANDLE_LENGTH 1
#define HANDLE_MAX_LENGTH 100
#define HEADER_LENGTH sizeof(struct normal_header)

#define CLIENT_INITIAL_PACKET 1
#define SERVER_ACCEPT_HANDLE 2
#define SERVER_REJECT_HANDLE 3
#define CLIENT_BROADCAST 4
#define CLIENT_MESSAGE 5
#define SERVER_HANDLE_OK 6
#define SERVER_HANDLE_ERROR 7
#define CLIENT_EXIT_REQUEST 8
#define SERVER_ACK_EXIT 9
#define CLIENT_HANDLE_REQUEST 10
#define SERVER_HANDLE_LIST 11

struct normal_header {
    uint32_t seq_number;
    uint8_t flag;
}__attribute__((packed)) ;

#endif
