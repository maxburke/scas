#ifndef SCAS_NET_H
#define SCAS_NET_H

#include <stdint.h>

/*
 * SCAS packets contain a header and a payload, the latter depends on the 
 * particular command being sent to the server. The header contains both
 * the size of the entire command as well as the command being sent.
 * All data is little endian.
 */

struct scas_header_t
{
    uint64_t packet_size;
    uint32_t command;
};

enum scas_command_t
{
    CMD_SNAPSHOT_LIST,
    CMD_SNAPSHOT_PUSH,
    CMD_SHAPSHOT_PULL,
    CMD_DATA_FETCH,
    CMD_DATA_PUSH
};

/*
 * Connects to the specified server. On success, returns a socket that can be 
 * read from/written to. On failure, returns < 0.
 */
int
scas_connect(const char *server_name);

#endif
