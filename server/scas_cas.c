/***********************************************************************
 * scas, Copyright (c) 2012-2013, Maximilian Burke
 * This file is distributed under the FreeBSD license. 
 * See LICENSE for details.
 ***********************************************************************/

/*
 * The _GNU_SOURCE define must be set in order to be able to use the 
 * MAP_ANONYMOUS flag to mmap.
 */
#define _GNU_SOURCE

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include "scas_base.h"
#include "scas_cas.h"

#define CACHE_ROOT "cache/"
#define FILENAME_SIZE ((sizeof(struct scas_hash_t) * 2) + sizeof(CACHE_ROOT))
#define CACHE_SIZE (size_t)0x100000000UL

struct scas_cas_entry_t *cache;
struct scas_cas_entry_t *cache_end;
struct scas_cas_entry_t *cache_alloc_limit;
struct scas_cas_entry_t *cache_limit;
long cache_counter;

void
scas_cas_cache_initialize(void)
{
    scas_mkdir(CACHE_ROOT);

    if (cache != NULL)
        return;

    cache = mmap(NULL, CACHE_SIZE, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    assert(cache != MAP_FAILED);

    cache_end = cache;
    cache_alloc_limit = cache;
    cache_limit = (struct scas_cas_entry_t *)((char *)cache + CACHE_SIZE);
}

static int
scas_cas_cache_comparator(const void *ptr_a, const void *ptr_b)
{
    const struct scas_cas_entry_t *a;
    const struct scas_cas_entry_t *b;

    a = ptr_a;
    b = ptr_b;

    return memcmp(&a->hash, &b->hash, sizeof(struct scas_hash_t));
}
    
static struct scas_cas_entry_t *
scas_cas_cache_find(struct scas_hash_t hash)
{
    struct scas_cas_entry_t temp;
    struct scas_cas_entry_t *entry;

    /*
     * TODO: Not thread-safe.
     */

    temp.hash = hash;
    entry = bsearch(&temp, cache, cache_end - cache, sizeof(struct scas_cas_entry_t), scas_cas_cache_comparator); 

    return entry;
}

static void
scas_cas_create_filename(char *filename, size_t filename_length, struct scas_hash_t hash)
{
    size_t i;
    size_t idx;
    char *hash_ptr;
    static const char hex_chars[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' };

    hash_ptr = (char *)&hash;

    idx = 0;
    for (i = 0; i < sizeof hash; ++i)
    {
        unsigned char c = (unsigned char)hash_ptr[i];
        unsigned char idx0 = (c >> 4) & 0xf;
        unsigned char idx1 = c & 0xf;

        filename[idx++] = hex_chars[idx0];

        if (idx == 2) 
        {
            filename[idx++] = '/';
        }

        filename[idx++] = hex_chars[idx1];
    }

    filename[idx++] = 0;

    assert(idx <= filename_length);
}

static struct scas_cas_entry_t *
scas_cas_find_destination_entry(struct scas_hash_t hash, struct scas_cas_entry_t *begin, struct scas_cas_entry_t *end)
{
    ssize_t index;
    ssize_t half;
    int result;

    if (begin == end)
    {
        return begin;
    }

    index = end - begin;
    assert(index >= 0);

    half = index / 2;
    result = memcmp(&begin[half].hash, &hash, sizeof(struct scas_hash_t));

    if (result == 1)
    {
        return scas_cas_find_destination_entry(hash, begin + half, end);
    }
    else if (result == 0)
    {
        assert(0 && "Uh, if these hashes are equal then something bad has happened.");
        abort();
    }
    else
    {
        return scas_cas_find_destination_entry(hash, begin, begin + half);
    }
}

static struct scas_cas_entry_t *
scas_cas_allocate_entry(struct scas_hash_t hash)
{
    struct scas_cas_entry_t *entry;

    /*
     * TODO: Not thread safe.
     */

    if (cache_end + 1 > cache_alloc_limit)
    {
        int result;
        long pagesize;

        pagesize = sysconf(_SC_PAGESIZE);
        result = mprotect(cache_alloc_limit, (size_t)pagesize, PROT_READ | PROT_WRITE);
        assert(result == 0);

        cache_alloc_limit = (struct scas_cas_entry_t *)((char *)cache_alloc_limit + pagesize);
    }

    entry = scas_cas_find_destination_entry(hash, cache, cache_end);

    /*
     * TODO: Yup, this is horrible. Make more efficient later.
     */
    memmove(entry, entry + 1, (cache_end - entry) * sizeof(struct scas_cas_entry_t));
    ++cache_end;

    /*
     * TODO: This should be atomic if we need to run multiple threads.
     */
    entry->id = cache_counter++;

    return entry;
}

int
scas_cas_contains(struct scas_hash_t hash)
{
    char filename[FILENAME_SIZE] = CACHE_ROOT;
    struct stat meta;
    int result;

    scas_cas_create_filename(filename + sizeof(CACHE_ROOT), sizeof filename, hash);
    result = stat(filename, &meta);

    return result == 0;
}

const struct scas_cas_entry_t *
scas_cas_read_acquire(struct scas_hash_t hash)
{
    struct scas_cas_entry_t *entry;
    char filename[FILENAME_SIZE] = CACHE_ROOT;
    int fd;
    struct stat meta;

    entry = scas_cas_cache_find(hash);
    if (entry != NULL)
    {
        ++entry->ref_count;

        return entry;
    }

    scas_cas_create_filename(filename + sizeof(CACHE_ROOT), sizeof filename, hash);
    fd = open(filename, O_RDONLY);

    if (fd < 0)
    {
        if (errno == ENFILE)
        {
            /*
             * TODO: Evict entries from the cache and try again.
             */
            BREAK();
            return scas_cas_read_acquire(hash);
        }

        return NULL;
    }

    if (fstat(fd, &meta) < 0)
    {
        return NULL;
    }

    /*
     * TODO: Not thread safe.
     */

    entry = scas_cas_allocate_entry(hash);
    entry->mem = mmap(NULL, meta.st_size, PROT_READ, MAP_SHARED, fd, 0);
    assert(entry->mem != MAP_FAILED);

    entry->fd = fd;
    entry->size = meta.st_size;
    ++entry->ref_count;

    return entry;
}

void
scas_cas_read_release(const struct scas_cas_entry_t *entry)
{
    struct scas_cas_entry_t *writable_entry;

    writable_entry = (struct scas_cas_entry_t *)entry;
    --writable_entry->ref_count;
}

struct scas_cas_entry_t *
scas_cas_begin_write(struct scas_hash_t hash, size_t size)
{
    static __thread struct scas_cas_entry_t entry;
    char filename[FILENAME_SIZE] = CACHE_ROOT;
    int fd;
    int result;

    assert(entry.mem == NULL);
    assert(entry.size == 0);
    assert(entry.fd == 0);

    scas_cas_create_filename(filename + sizeof(CACHE_ROOT), sizeof filename, hash);
    fd = open(filename, O_RDWR);
    assert(fd >= 0);

    result = ftruncate(fd, size);
    assert(result == 0);

    entry.mem = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    assert(entry.mem != MAP_FAILED);

    entry.hash = hash;
    entry.fd = fd;
    entry.size = size;

    return &entry;
}

void
scas_cas_end_write(struct scas_cas_entry_t *entry)
{
    struct scas_cas_entry_t *cache_entry;
    int result;

    /*
     * After the write has finished the memory is marked as read-only to
     * prevent any unfortunate side-effects from mangling it.
     */
    result = mprotect(entry->mem, entry->size, PROT_READ);
    assert(result == 0);

    cache_entry = scas_cas_allocate_entry(entry->hash);
    cache_entry->hash = entry->hash;
    cache_entry->mem = entry->mem;
    cache_entry->fd = entry->fd;
    cache_entry->size = entry->size;

    memset(entry, 0, sizeof(struct scas_cas_entry_t));
}

