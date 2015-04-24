#ifndef _packets_h
#define _packets_h

#include <stdint.h>

#define BUFF_SIZE 1440
#define MESSAGE_LENGTH 1000
#define HANDLE_LENGTH 1
#define HEADER_LENGTH sizeof(struct normal_header)

struct normal_header {
    uint32_t seq_number;
    uint8_t flag;
}__attribute__((packed)) ;

#endif
