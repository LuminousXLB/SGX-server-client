#ifndef __NET_UTILS_H_SJM__
#define __NET_UTILS_H_SJM__

#include "csapp.h"
#define LENGTH_FIELD_BYTES sizeof(uint32_t)

int64_t read_socket(int sockfd, uint8_t *buffer, uint32_t buffer_max_len);
void write_socket(int sockfd, uint8_t *buffer, uint32_t length);

void hexdump(FILE *stream, uint8_t const *data, uint32_t len);

#endif //__NET_UTILS_H_SJM__