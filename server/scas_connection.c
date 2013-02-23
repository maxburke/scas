#define _POSIX_SOURCE

#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include "scas_base.h"
#include "scas_cas.h"
#include "scas_connection.h"
#include "scas_linear_allocator.h"
#include "scas_meta.h"
#include "scas_net.h"

#define POOL_REALLOCATION_DELTA 16
#define SNAPSHOT_ROOT "snapshot/"

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
    struct scas_linear_allocator_t *allocator;
    void *context;
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
scas_connection_reset(struct scas_connection_t *connection)
{
    /*
     * Resets the connection object without closing the socket fd.
     */

    memset(&connection->header, 0, sizeof(struct scas_header_t));
    scas_linear_allocator_destroy(connection->allocator);

    connection->allocator = NULL;
    connection->context = NULL;
    connection->ptr = NULL;
    connection->size = 0;
    connection->offset = 0;
}

static void
scas_connection_free(struct scas_connection_t *connection)
{
    scas_connection_list_remove(connection);

    scas_linear_allocator_destroy(connection->allocator);
    memset(connection, 0, sizeof(struct scas_connection_t));

    scas_connection_list_add(&free_list_anchor, connection);
}

void
scas_connection_initialize(void)
{
    scas_mkdir(SNAPSHOT_ROOT);

    free_list_anchor.next = &free_list_anchor;
    free_list_anchor.prev = &free_list_anchor;

    live_list_anchor.next = &live_list_anchor;
    live_list_anchor.prev = &live_list_anchor;
}

#define SCAS_CONNECTION_DEFINE_OP(OP)                                       \
    static int                                                              \
    scas_connection_ ## OP(struct scas_connection_t *connection)            \
    {                                                                       \
        ssize_t result;                                                     \
        char *ptr;                                                          \
        size_t offset;                                                      \
        size_t size;                                                        \
        size_t nbytes;                                                      \
                                                                            \
        offset = connection->offset;                                        \
        size = connection->size;                                            \
        ptr = (char *)connection->ptr + offset;                             \
        nbytes = size - offset;                                             \
                                                                            \
        result = OP(connection->fd, ptr, nbytes);                           \
                                                                            \
        if (result >= 0)                                                    \
        {                                                                   \
            size_t new_offset = offset + (size_t)result;                    \
            connection->offset = new_offset;                                \
            return new_offset != size;                                      \
        }                                                                   \
                                                                            \
        /*                                                                  \
         * The only error codes we should see here should be EAGAIN/EWOULDBLOCK. \
         * All else are "unexpected".                                       \
         */                                                                 \
        assert(errno == EAGAIN || errno == EWOULDBLOCK);                    \
        return 1;                                                           \
    }

SCAS_CONNECTION_DEFINE_OP(read)
SCAS_CONNECTION_DEFINE_OP(write)

struct scas_recursion_context_t
{
    uint32_t num_entries;
    uint32_t current_idx;
    int visited;
};

struct scas_fetch_packet_t
{
    struct scas_header_t header;
    struct scas_hash_t hash;
};

struct scas_snapshot_push_context_t
{
    struct scas_file_meta_t snapshot_meta;
    struct scas_hash_t current_dir_record;
    struct scas_header_t push_header;
    struct scas_fetch_packet_t fetch_packet;
    struct scas_cas_entry_t *cas_entry;
    int have_root;
    int depth;
    int state;
    struct scas_recursion_context_t stack[1024];
};

static struct hash_t
scas_get_parent(struct scas_hash_t hash)
{
    const struct scas_cas_entry_t *cas_entry;
    struct scas_directory_meta_t *meta;

    cas_entry = scas_cas_read(hash);
    meta = cas_entry->mem;

    return meta->parent;
}

static struct scas_snapshot_push_context_t *
scas_initialize_snapshot_push_context(struct scas_connection_t *connection)
{
    struct scas_linear_allocator_t *allocator;
    struct scas_snapshot_push_context_t *context;

    if (connection->context != NULL)
    {
        return connection->context;
    }

    assert(connection->allocator == NULL);
    allocator = scas_linear_allocator_create(DEFAULT_ALLOCATOR_SIZE);

    context = scas_linear_allocator_alloc(allocator, sizeof(struct scas_snapshot_push_context_t));
    context->have_root = 0;

    connection->context = context;
    connection->allocator = allocator;

    connection->ptr = &context->snapshot_meta;
    connection->size = sizeof(struct scas_snapshot_push_context_t);
    connection->offset = 0;

    return context;
}

static int
scas_snapshot_push_iterate(struct scas_connection_t *connection)
{
    /*
     * When we enter this function we can be in one of a number of states:
     *   1) Writing a FETCH_DATA request for a directory record
     *   2) Writing a FETCH_DATA request for a blob
     *   4) Reading the results of a FETCH_DATA request for a directory
     *      record.
     *   6) Reading the results of a FETCH_DATA request for a blob
     */

    /*
     * Workflow:
     * 1) Check CAS for record. 
     *    a) If it exists, we're done, and can back up the tree.
     *    b) If it doesn't exist, continue.
     * 2) Fetch directory record.
     * 3) Fetch
     */

    /* 
     * When do we return? When we're "blocked" on a read or write.
     */

    enum scas_snapshot_push_iterate_state_t
    {
        INITIAL,
        FETCHING_DIRECTORY,
        READING_DIRECTORY_HEADER,
        READING_DIRECTORY,
        ITERATING_OVER_DIRECTORY,
        FETCHING_FILE
    };

    struct scas_snapshot_push_context_t *context;
    enum scas_snapshot_push_iterate_state_t state;

    context = connection->context;
    state = context->state;

    for (;;)
    {
        if (state == INITIAL)
        {
            if (scas_cas_contains(context->current_dir_record))
            {
                goto up_one_level;
            }

            state = FETCHING_DIRECTORY;
        }

        if (state == FETCHING_DIRECTORY)
        {
            if (connection->ptr == NULL)
            {
                context->fetch_packet.header.packet_size = sizeof(struct scas_header_t) + sizeof(struct scas_hash_t);
                context->fetch_packet.header.command = CMD_DATA_FETCH;
                context->fetch_packet.hash = context->current_dir_record;

                connection->ptr = &context->fetch_packet;
                connection->offset = 0;
                connection->size = sizeof(struct scas_fetch_packet_t);
            }

            if (scas_connection_write(connection) != 0)
            {
                goto save_state_and_yield;
            }

            connection->ptr = &context->push_header;
            connection->offset = 0;
            connection->size = sizeof(struct scas_header_t);

            state = READING_DIRECTORY_HEADER;
        }

        if (state == READING_DIRECTORY_HEADER)
        {
            if (scas_connection_read(connection) != 0)
            {
                goto save_state_and_yield;
            }

            context->cas_entry = scas_cas_begin_write(context->current_dir_record, scas_header_payload_size(context->push_header));
            connection->ptr = context->cas_entry->mem;
            connection->offset = 0;
            connection->size = context->cas_entry->size;
            state = READING_DIRECTORY;
        }

        if (state == READING_DIRECTORY)
        {
            struct scas_cas_entry_t *cas_entry;
            struct scas_directory_meta_t *directory_entry;
            struct scas_recursion_context_t *stack;

            if (scas_connection_read(connection) != 0)
            {
                goto save_state_and_yield;
            }

            cas_entry = context->cas_entry;
            directory_entry = cas_entry->mem;

            stack = &context->stack[context->depth];
            stack->num_entries = directory_entry->num_entries;
            stack->current_idx = 0;
            
            scas_cas_end_write(cas_entry);
            state = ITERATING_OVER_DIRECTORY;
        }

        if (state == ITERATING_OVER_DIRECTORY)
        {
            uint32_t num_entries;
            uint32_t i;
            const struct scas_cas_entry_t *directory;
            struct scas_directory_meta_t *directory_entry;
            struct scas_file_meta_t *meta;
            struct scas_recursion_context_t *stack = &context->stack[context->depth];

            directory = scas_cas_read(context->current_dir_record);
            directory_entry = directory->mem;
            meta = scas_get_file_meta_base(directory_entry);

            for (i = stack->current_idx, num_entries = stack->num_entries; i < num_entries; ++i)
            {
                struct scas_file_meta_t *meta;

                if (scas_cas_contains(meta[i].content))
                {
                    continue;
                }

                if (scas_is_directory(meta[i].flags))
                {
                    context->depth++;
                    context->current_dir_record = meta[i].content;
                    state = INITIAL;
                    break;
                }
                else
                {
                    state = FETCHING_FILE;
                    break;
#error NOT DONE YET
                }
            }

            if (i == num_entries)
            {
                goto up_one_level;
            }
        }

        if (state == FETCHING_FILE)
        {
#error OR HERE
        }

    up_one_level:
        context->current_dir_record = scas_get_parent(context->current_dir_record);
        --context->depth;
        state = ITERATING_OVER_DIRECTORY;
        continue;

    save_state_and_yield:
#error ha
        context->state = state;
        return 0;
    }
}

int
scas_connection_handle_snapshot_push(struct scas_connection_t *connection)
{
    struct scas_snapshot_push_context_t *context;

    /*
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
     * In the case of the meta blob pushed as a part of the SNAPSHOT_PUSH
     * handshake, the content field points to the root entry hash.
     *
     *                      SNAPSHOT_PUSH ->
     *       struct scas_file_meta_t meta ->
     *                                    <- DATA_FETCH 
     *                                    <- struct scas_hash_t hash
     *             data (gzip compressed) ->
     *                                   ....
     *                                    <- DATA_FETCH ...
     *                                    <- struct scas_hash_t hash
     *             data (gzip compressed) ->
     */

    context = scas_initialize_snapshot_push_context(connection);

    if (!context->have_root)
    {
        if (scas_connection_read(connection) != 0)
            return 0;
        
        context->have_root = 1;
        context->current_dir_record = context->snapshot_meta.content;

        /*
         * If a snapshot is being pushed we should *not* have it in the CAS
         * already, but we should check just to be sure.
         */
        if (scas_cas_contains(context->current_dir_record))
        {
            scas_connection_reset(connection);

            return 0;
        }
    }

    return scas_snapshot_push_iterate(connection);
}


static int
scas_connection_handle_snapshot_pull(struct scas_connection_t *connection)
{
    /*
     *    SNAPSHOT_PULL ->
     * snapshot_name[0] ->
     *                  <- struct scas_directory_meta_t root
     */
    UNUSED(connection);

    return 0;
}

static int
scas_connection_handle_data_fetch(struct scas_connection_t *connection)
{
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
    UNUSED(connection);

    return 0;
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
        case CMD_SNAPSHOT_PUSH:
            return scas_connection_handle_snapshot_push(connection);
        case CMD_SNAPSHOT_PULL:
            return scas_connection_handle_snapshot_pull(connection);
        case CMD_DATA_FETCH:
            return scas_connection_handle_data_fetch(connection);
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
            connection->context = &connection->header;
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

