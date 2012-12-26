#ifndef SCAS_NET_H
#define SCAS_NET_H

#include <stddef.h>

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

long
scas_write(int connection, enum scas_command_t command, const void *data, size_t data_size);

void *
scas_read(int connection);

#endif
