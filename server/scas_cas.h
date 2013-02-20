#ifndef SCAS_CAS_H
#define SCAS_CAS_H

#include "scas_base.h"

struct scas_cas_entry_t
{
    struct scas_hash_t hash;
    void *mem;
    int fd;
    size_t size;
    long id;
};

void
scas_cas_cache_initialize(void);

const struct scas_cas_entry_t *
scas_cas_read(struct scas_hash_t hash);

int
scas_cas_contains(struct scas_hash_t hash);

struct scas_cas_entry_t *
scas_cas_begin_write(struct scas_hash_t hash, size_t size);

void
scas_cas_end_write(struct scas_cas_entry_t *entry);

#endif
