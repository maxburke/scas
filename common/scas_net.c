#include <errno.h>
#include <netdb.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>

#include "scas_base.h"
#include "scas_net.h"

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

int
scas_connect(const char *server_name)
{
    struct addrinfo *addrinfo, *p;
    int sockfd;
    int success = 0;

    if (getaddrinfo(server_name, NULL, NULL, &addrinfo))
    {
        scas_log("Could not look up server %s", server_name);
        return -1;
    }

    for (p = addrinfo; p; p = p->ai_next)
    {
        if (p->ai_socktype != SOCK_STREAM)
            continue;

        sockfd = connect(p->ai_family, p->ai_addr, p->ai_protocol);

        if (sockfd < 0)
            continue;

        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == 0)
        {
            success = 1;
            break;
        }
    }

    freeaddrinfo(addrinfo);

    if (!success)
    {
        scas_log("Unable to open connection to server %s.", server_name);
        return -1;
    }

    return sockfd;
}

static void
loop_write(int connection, const void *data, size_t data_size)
{
    ssize_t data_written;
    ssize_t data_remaining;

    data_written = 0;
    data_remaining = (long)data_size;

    while (data_remaining > 0)
    {
        ssize_t rv;

        rv = write(connection, ((char *)data + data_written), data_remaining);

        if (rv < 0)
        {
            scas_log("Unable to write data of size %d. Error code %d returned.", data_size, errno);
            return;
        }

        data_written += rv;
        data_remaining -= rv;
    }
}

static void
loop_read(int connection, void *data, size_t data_size)
{
    ssize_t data_read;
    ssize_t data_remaining;

    data_read = 0;
    data_remaining = (long)data_size;

    while (data_remaining > 0)
    {
        ssize_t rv;

        rv = read(connection, ((char *)data + data_read), data_remaining);

        if (rv < 0)
        {
            scas_log("Unable to read data of size %d. Error code %d returned.", data_size, errno);
            return;
        }

        data_read += rv;
        data_remaining -= rv;
    }
}

long
scas_write(int connection, enum scas_command_t command, const void *data, size_t data_size)
{
    struct scas_header_t header;

    header.packet_size = data_size;
    header.command = command;

    loop_write(connection, &header, sizeof(struct scas_header_t));
    loop_write(connection, data, data_size);

    return 0;
}

void *
scas_read(int connection)
{
    struct scas_header_t header;
    void *mem;

    loop_read(connection, &header, sizeof(struct scas_header_t));
    mem = malloc(header.packet_size);
    loop_read(connection, mem, header.packet_size);
    
    return mem;
}


