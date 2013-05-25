/***********************************************************************
 * scas, Copyright (c) 2012-2013, Maximilian Burke
 * This file is distributed under the FreeBSD license. 
 * See LICENSE for details.
 ***********************************************************************/

#include <errno.h>
#include <netdb.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include "scas_base.h"
#include "scas_net.h"

#define SCAS_PORT 15240

#define SCAS_VERIFY(x, str) if (!(x)) { scas_log_system_error(str); return -1; } else (void)0

int
scas_listen(void)
{
    int socket_fd;
    struct sockaddr_in addr;
    const int LISTEN_BACKLOG = 50;

    memset(&addr, 0, sizeof addr);
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
   
    SCAS_VERIFY(socket_fd != 0, "Could not create socket");

    addr.sin_port = htons(SCAS_PORT);
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_family = AF_INET;

    SCAS_VERIFY(bind(socket_fd, (struct sockaddr *)&addr, sizeof addr) == 0, "Could not bind socket");
    SCAS_VERIFY(listen(socket_fd, LISTEN_BACKLOG) == 0, "Could not listen on socket");

    return socket_fd;
}

int
scas_accept(int socket_fd)
{
    int socket;
    struct sockaddr_in connection;
    socklen_t size;

    size = sizeof connection;
    socket = accept(socket_fd, (struct sockaddr *)&connection, &size);
    VERIFY(socket != -1);

    return socket;
}

int
scas_connect(const char *server_name)
{
    struct addrinfo *addrinfo, *p;
    int socket_fd;
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

        socket_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);

        if (socket_fd < 0)
            continue;

        switch (p->ai_addr->sa_family)
        {
            case AF_INET:
                {
                    struct sockaddr_in *s;

                    s = (struct sockaddr_in *)p->ai_addr;
                    s->sin_port = htons(SCAS_PORT);
                }
                break;
            case AF_INET6:
                {
                    struct sockaddr_in6 *s;

                    s = (struct sockaddr_in6 *)p->ai_addr;
                    s->sin6_port = htons(SCAS_PORT);
                }
                break;
            default:
                BREAK();
                break;
        }

        if (connect(socket_fd, p->ai_addr, p->ai_addrlen) == 0)
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

    return socket_fd;
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


