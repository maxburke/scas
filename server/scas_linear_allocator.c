#define _GNU_SOURCE

#include <assert.h>
#include <stddef.h>
#include <unistd.h>
#include <sys/mman.h>

#include "scas_linear_allocator.h"

static size_t pagesize;

struct scas_linear_allocator_t
{
    void *ptr;
    void *enabled_region_end;
    void *heap_limit;
};

static void *
scas_enable_pages(void *memory, size_t size)
{
    size_t rounded_size;
    size_t pagesize_mask;
    int result;

    pagesize_mask = pagesize - 1;
    rounded_size = (size + pagesize_mask) & ~(pagesize_mask);
    result = mprotect(memory, rounded_size, PROT_READ | PROT_WRITE);
    assert(result == 0);

    return (char *)memory + rounded_size;
}

struct scas_linear_allocator_t *
scas_linear_allocator_create(size_t max_size)
{
    struct scas_linear_allocator_t *allocator;
    void *enabled_region_end;

    /*
     * Imposing the condition that the max_size should be a power of two.
     */
    assert((max_size & (max_size - 1)) == 0);

    allocator = mmap(NULL, max_size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    assert(allocator != MAP_FAILED);

    if (pagesize == 0)
    {
        pagesize = (size_t)sysconf(_SC_PAGESIZE);
    }

    assert(sizeof(struct scas_linear_allocator_t) < pagesize);

    enabled_region_end = scas_enable_pages(allocator, sizeof(struct scas_linear_allocator_t));

    allocator->ptr = &allocator[1];
    allocator->enabled_region_end = enabled_region_end;
    allocator->heap_limit = (char *)allocator + max_size;

    return allocator;
}

void *
scas_linear_allocator_alloc(struct scas_linear_allocator_t *allocator, size_t size)
{
    void *mem;
    void *new_top;
    void *enabled_region_end;
    size_t aligned_size;

    aligned_size = (size + sizeof(void *) - 1) & ~(sizeof(void *) - 1);
    mem = allocator->ptr;
    new_top = (char *)mem + aligned_size;

    /*
     * Ensure the memory doesn't escape the allocator. 
     * TODO: Grow the pool if it does.
     */
    assert(new_top < allocator->heap_limit);

    enabled_region_end = allocator->enabled_region_end;
    if (new_top > enabled_region_end)
    {
        size_t bytes_to_enable;

        bytes_to_enable = (char *)new_top - (char *)enabled_region_end;
        allocator->enabled_region_end = scas_enable_pages(enabled_region_end, bytes_to_enable);
    }

    allocator->ptr = new_top;

    return mem;
}

void
scas_linear_allocator_destroy(struct scas_linear_allocator_t *allocator)
{
    size_t size;
    int result;

    if (allocator == NULL)
    {
        return;
    }

    size = (char *)allocator->heap_limit - (char *)allocator;
    result = munmap(allocator, size);
    assert(result == 0);
}

