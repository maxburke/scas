#ifndef SCAS_META_H
#define SCAS_META_H

#include "scas_base.h"

struct scas_file_meta_t
{
    uint64_t timestamp;
    uint64_t size;
    uint32_t flags;
    struct scas_hash_t content;
};

/*
 * Directory entries are setup as following:
 * offset  data
 * 0                                        struct scas_directory_t
 * sizeof(struct scas_directory_t)          file metadata (struct scas_file_meta_t)
 * " + n * sizeof(struct scas_file_meta_t)  name blob indices
 * " + n * sizeof(uint32_t)                 names
 */

struct scas_directory_meta_t
{
    uint32_t num_entries;
    struct scas_hash_t parent;
};

static inline struct scas_file_meta_t *
scas_get_file_meta_base(struct scas_directory_meta_t *directory)
{
    void *file_meta_base;
    file_meta_base = &directory[1];

    return (struct scas_file_meta_t *)file_meta_base;
}

#endif
