#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "scas_base.h"
#include "scas_connection.h"
#include "scas_net.h"

#define POOL_REALLOCATION_DELTA 16

enum connection_state_t
{
    NEW,
    RECEIVING_HEADER
};

struct scas_connection_t
{
    struct scas_connection_t *next;
    struct scas_connection_t *prev;
    struct scas_header_t header;
    int fd;
    enum connection_state_t state;
    void *ptr;
    uint64_t offset;
    uint64_t size;
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

static int
scas_connection_read(struct scas_connection_t *connection)
{
    ssize_t result;
    char *ptr;
    size_t offset;
    size_t size;
    size_t nbytes;

    offset = connection->offset;
    size = connection->size;
    ptr = (char *)connection->ptr + offset;
    nbytes = size - offset;

    result = read(connection->fd, ptr, nbytes);

    if (result >= 0)
    {
        size_t new_offset = offset + (size_t)result;
        connection->offset = new_offset;
        return new_offset != size;
    }

    /*
     * The only error codes we should see here should be EAGAIN/EWOULDBLOCK.
     * All else are "unexpected".
     */
    assert(errno == EAGAIN || errno == EWOULDBLOCK);
    return 1;
}

static int
scas_connection_process_command(struct scas_connection_t *connection)
{
    switch (connection->header.command)
    {
        /* 
         * The message flow for the commands is documented as 
         *   client ->
         * for messages being sent to the server from the client; or
         *          <- server
         * for messages being sent to the client from the server; and
         *          XX
         * for end of session.
         */
        case CMD_SNAPSHOT_LIST:
            /*
             * struct scas_snapshot_t 
             * {
             *     uint64_t timestamp;
             *     uint32_t name_length;
             *     char name[0];
             * };
             *
             *   SNAPSHOT_LIST ->
             *                 <- list of scas_snapshot_t structures
             *
             * The name_length field is exclusive of the null termination byte
             * that the name field includes.
             */
            BREAK();
            break;
        case CMD_SNAPSHOT_PUSH:
            /*
             * These data structures are used for both the SNAPSHOT_PUSH and
             * SNAPSHOT_PULL operations:
             *
             * struct scas_file_meta_t
             * {
             *     uint64_t timestamp;
             *     uint64_t size;
             *     uint32_t flags;
             *     struct scas_hash_t content_id;
             * };
             *
             * struct scas_directory_entry_t
             * {
             *     uint32_t nentries;
             *     struct scas_file_meta_t meta[0];
             *     uint32_t name_blob_indices[0];
             *     char name_blob[0];
             * };
             *
             * The arrays meta/name_blob_indices/name_blob all have nentries
             * entries in them. The name_blob field consists of all the names
             * for files in the directory, null terminated, jammed together.
             * The indices for the start of each name are stored in 
             * name_blob_indices, meaning that to index the seventh name in 
             * the directory entry, you would use the expression
             * name_blog[name_blob_indices[6]].
             *
             * The client only sends the root node of the snapshot. The server
             * will iterate over the contained files in the root node and its
             * descendents to verify the contents. For content it does not
             * have, the server will issue DATA_FETCH commands to the client.
             *
             *                      SNAPSHOT_PUSH ->
             *        struct scas_snapshot_t meta ->
             * struct scas_directory_entry_t root ->
             *                                    <- DATA_FETCH 
             *                                    <- struct scas_hash_t hash
             *             data (gzip compressed) ->
             *                                   ....
             *                                    <- DATA_FETCH ...
             *                                    <- struct scas_hash_t hash
             *             data (gzip compressed) ->
             */
            BREAK();
            break;
        case CMD_SNAPSHOT_PULL:
            /*
             * SNAPSHOT_PULL ->
             *               <- struct scas_directory_entry_t root
             */
            BREAK();
            break;
        case CMD_DATA_FETCH:
            /*
             * struct scas_hash_t
             * {
             *     uint32_t hash[5];
             * };
             *
             *              DATA_FETCH ->
             * struct scas_hash_t hash ->
             *                         <- data (gzip compressed)
             */
            BREAK();
            break;
        case CMD_QUIT:
            /*
             * By returning non-zero from here to the main loop the session
             * will be terminated and the socket closed.
             */
            scas_connection_free(connection);
            return 1;
        default:
            assert(0 && "Garbled command field.");
    }

    return 0;
}

static int
scas_connection_iterate(struct scas_connection_t *connection)
{
    switch (connection->state)
    {
        case NEW:
            connection->ptr = &connection->header;
            connection->size = sizeof(struct scas_header_t);
            connection->state = RECEIVING_HEADER;
            /*
             * Deliberate fall-through to the RECEIVING_HEADER case.
             */
        case RECEIVING_HEADER:
            if (scas_connection_read(connection) != 0)
            {
                return 0;
            }

        default:
            return scas_connection_process_command(connection);
            break;
    }
}

int
scas_handle_connection(int fd)
{
    struct scas_connection_t *connection;

    connection = scas_connection_find(fd);

    return scas_connection_iterate(connection);
}

