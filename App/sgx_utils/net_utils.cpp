#include "net_utils.h"

void PRINT_BYTE_ARRAY(FILE *file, uint8_t *array, uint32_t len)
{
    if (!array || !len)
    {
        fprintf(file, "\n( null )\n");
        return;
    }

    fprintf(file, "%u bytes:\n{\n", len);

    uint32_t i = 0;
    for (i = 0; i < len - 1; i++)
    {
        fprintf(file, "%0x, ", array[i]);
        if (i % 16 == 15)
        {
            fprintf(file, "\n");
        }
        else if (i % 8 == 7)
        {
            fprintf(file, " ");
        }
    }
    fprintf(file, "%0x ", array[i]);
    fprintf(file, "\n}\n");
}

int64_t read_socket(int sockfd, uint8_t *buffer, uint32_t buffer_max_len)
{
    uint32_t length;
    int64_t ret = recv(sockfd, (void *)&length, LENGTH_FIELD_BYTES, MSG_WAITALL);
    if (ret < 0)
    {
        unix_error("read_socket error when reading length field");
    }

    be32toh(length);

    if (buffer_max_len < length)
    {
        return -1;
    }

    ret = recv(sockfd, buffer, length, MSG_WAITALL);
    if (ret < length)
    {
        if (length == 0x0C)
        {
            return ret;
        }
        else if (ret < 0)
        {
            unix_error("read_socket error when reading payload");
        }
        else
        {
            unix_error("read_socket didn't get sufficient data");
        }
    }

    PRINT_BYTE_ARRAY(stdout, buffer, length);

    return ret;
}

void write_socket(int sockfd, uint8_t *buffer, uint32_t length)
{
    PRINT_BYTE_ARRAY(stdout, buffer, length);

    uint32_t be_length = length;
    htobe32(be_length);

    if (send(sockfd, (void *)&be_length, LENGTH_FIELD_BYTES, 0) != LENGTH_FIELD_BYTES)
    {
        unix_error("write_socket error when writing length field");
    }

    if (send(sockfd, (void *)buffer, length, 0) != length)
    {
        unix_error("write_socket error when writing payload");
    }
}