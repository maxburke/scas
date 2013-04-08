/***********************************************************************
 * scas, Copyright (c) 2012-2013, Maximilian Burke
 * This file is distributed under the FreeBSD license. 
 * See LICENSE for details.
 ***********************************************************************/

#ifndef SCAS_NET_H
#define SCAS_NET_H

#include <stddef.h>
#include <stdint.h>

enum scas_command_t
{
    CMD_SNAPSHOT_PUSH,
    CMD_DATA_FETCH,
    CMD_DATA,
    CMD_QUIT
};

/*
 * SCAS packets contain a header and a payload, the latter depends on the 
 * particular command being sent to the server. The header contains both
 * the size of the command as well as the command being sent. 
 *
 * All data is little endian.
 */

struct scas_header_t
{
    uint64_t packet_size;
    uint32_t command;
};

static inline uint64_t
scas_header_payload_size(struct scas_header_t header)
{
    return header.packet_size - sizeof(struct scas_header_t);
}

int
scas_listen(void);

int
scas_accept(int socket_fd);

/*
 * Connects to the specified server. On success, returns a socket that can be 
 * read from/written to. On failure, returns < 0.
 */
int
scas_connect(const char *server_name);

long
scas_write(int connection, enum scas_command_t command, const void *data, size_t data_size);

void *
scas_read(int connection);

#endif
