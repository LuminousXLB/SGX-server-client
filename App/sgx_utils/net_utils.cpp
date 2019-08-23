#include "net_utils.h"

/*
 * Copyright (C) 1998  Mark Baysinger (mbaysing@ucsd.edu)
 * Copyright (C) 1998,1999  Ross Combs (rocombs@cs.nmsu.edu)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

void hexdump(FILE *stream, uint8_t const *data, uint32_t len)
{
    unsigned int i;
    unsigned int r, c;

    if (!stream)
        return;
    if (!data)
        return;

    for (r = 0, i = 0; r < (len / 16 + (len % 16 != 0)); r++, i += 16)
    {
        fprintf(stream, "%04X:   ", i); /* location of first byte in line */

        for (c = i; c < i + 8; c++) /* left half of hex dump */
            if (c < len)
                fprintf(stream, "%02X ", ((unsigned char const *)data)[c]);
            else
                fprintf(stream, "   "); /* pad if short line */

        fprintf(stream, "  ");

        for (c = i + 8; c < i + 16; c++) /* right half of hex dump */
            if (c < len)
                fprintf(stream, "%02X ", ((unsigned char const *)data)[c]);
            else
                fprintf(stream, "   "); /* pad if short line */

        fprintf(stream, "   ");

        for (c = i; c < i + 16; c++) /* ASCII dump */
            if (c < len)
                if (((unsigned char const *)data)[c] >= 32 &&
                    ((unsigned char const *)data)[c] < 127)
                    fprintf(stream, "%c", ((char const *)data)[c]);
                else
                    fprintf(stream, "."); /* put this for non-printables */
            else
                fprintf(stream, " "); /* pad if short line */

        fprintf(stream, "\n");
    }

    fflush(stream);
}

////////////////////////////////////////////////////////////////////////////////

int64_t read_socket(int sockfd, uint8_t *buffer, uint32_t buffer_max_len)
{
    uint32_t length;
    int64_t ret = recv(sockfd, (void *)&length, LENGTH_FIELD_BYTES, MSG_WAITALL);
    if (ret < 0)
    {
        unix_error("read_socket error when reading length field");
    }
    else
    {
        hexdump(stdout, (uint8_t *)&length, LENGTH_FIELD_BYTES);
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
    else
    {

        hexdump(stdout, buffer, length);
    }
    return ret;
}

void write_socket(int sockfd, uint8_t *buffer, uint32_t length)
{
    uint32_t be_length = length;
    htobe32(be_length);

    if (send(sockfd, (void *)&be_length, LENGTH_FIELD_BYTES, 0) != LENGTH_FIELD_BYTES)
    {
        unix_error("write_socket error when writing length field");
    }
    else
    {
        hexdump(stdout, (uint8_t *)&be_length, LENGTH_FIELD_BYTES);
    }

    if (send(sockfd, (void *)buffer, length, 0) != length)
    {
        unix_error("write_socket error when writing payload");
    }
    else
    {
        hexdump(stdout, buffer, length);
    }
}