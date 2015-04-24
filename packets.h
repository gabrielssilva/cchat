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
#define CLIENT_EXITING 8
#define CLEINT_HANDLE_REQUEST 10

struct normal_header {
    uint32_t seq_number;
    uint8_t flag;
}__attribute__((packed)) ;

#endif
