#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "scas_base.h"
#include "scas_connection.h"
#include "scas_net.h"

#define POOL_REALLOCATION_DELTA 16

struct scas_connection_t
{
    struct scas_connection_t *next;
    struct scas_connection_t *prev;
    int fd;
    enum scas_command_t command;
    void *ptr;
    size_t offset;
};

struct scas_connection_t free_list_anchor;
struct scas_connection_t live_list_anchor;

static void
scas_connection_list_remove(struct scas_connection_t *connection)
{
    assert(connection != NULL);

    if (connection->next != NULL)
        connection->next->prev = connection->prev;

    if (connection->prev != NULL)
        connection->prev->next = connection->next;

    connection->next = NULL;
    connection->prev = NULL;
}

static void
scas_connection_list_add(struct scas_connection_t *list, struct scas_connection_t *connection)
{
    assert(list != NULL);
    assert(connection != NULL);
    assert(list != connection);

    connection->next = list->next;
    connection->prev = list;
    list->next->prev = connection;
    list->next = connection;
}

static struct scas_connection_t *
scas_connection_list_pop(struct scas_connection_t *list)
{
    struct scas_connection_t *connection;

    if (list->next == list)
        return NULL;

    connection = list->next;
    scas_connection_list_remove(connection);

    return connection;
}

static int
scas_connection_list_is_empty(struct scas_connection_t *list)
{
    assert(list != NULL);

    return list->next == list && list->prev == list;
}

static struct scas_connection_t *
scas_connection_allocate(void)
{
    struct scas_connection_t *connection;

    if (scas_connection_list_is_empty(&free_list_anchor))
    {
        struct scas_connection_t *slab;
        int i;

        slab = calloc(POOL_REALLOCATION_DELTA, sizeof(struct scas_connection_t));
        for (i = 0; i < POOL_REALLOCATION_DELTA; ++i)
        {
            scas_connection_list_add(&free_list_anchor, &slab[i]);
        }

        return scas_connection_allocate();
    }

    connection = free_list_anchor.next;
    scas_connection_list_remove(connection);

    return connection;
}

static struct scas_connection_t *
scas_connection_find(int fd)
{
    struct scas_connection_t *connection;
    struct scas_connection_t *i;

    for (i = &live_list_anchor; i != NULL; i = i->next)
    {
        if (i->fd == fd)
        {
            return i;
        }
    }

    connection = scas_connection_allocate();
    assert(connection != NULL);

    connection->fd = fd;
    scas_connection_list_add(&live_list_anchor, connection);

    return connection;
}

static void
scas_connection_free(struct scas_connection_t *connection)
{
    scas_connection_list_remove(connection);
    memset(connection, 0, sizeof(struct scas_connection_t));
    scas_connection_list_add(&free_list_anchor, connection);
}

void
scas_connection_initialize(void)
{
    free_list_anchor.next = &free_list_anchor;
    free_list_anchor.prev = &free_list_anchor;

    live_list_anchor.next = &live_list_anchor;
    live_list_anchor.prev = &live_list_anchor;
}

int
scas_handle_connection(int fd)
{
    struct scas_connection_t *connection;
    struct scas_header_t header;

    connection = scas_connection_find(fd);

    UNUSED(header);
    UNUSED(connection);
}

