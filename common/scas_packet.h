#ifndef SCAS_PACKET_H
#define SCAS_PACKET_H

#include <stdint.h>

/*
 * SCAS packets contain a header and a payload, the latter depends on the 
 * particular command being sent to the server. The header contains both
 * the size of the entire command as well as the command being sent. The
 * packet size includes the header, so this field should always be at least
 * 12.
 *
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

#endif
