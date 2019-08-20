#include "csapp.h"

#define LENGTH_FIELD_BYTES sizeof(uint8_t)

int64_t read_socket(int sockfd, uint8_t *buffer, uint32_t buffer_max_len);
void write_socket(int sockfd, uint8_t *buffer, uint32_t length);
