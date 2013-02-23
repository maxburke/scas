/***********************************************************************
 * scas, Copyright (c) 2012-2013, Maximilian Burke
 * This file is distributed under the FreeBSD license. 
 * See LICENSE for details.
 ***********************************************************************/

#ifndef SCAS_LINEAR_ALLOCATOR_H
#define SCAS_LINEAR_ALLOCATOR_H

#define DEFAULT_ALLOCATOR_SIZE  65536

struct scas_linear_allocator_t;

struct scas_linear_allocator_t *
scas_linear_allocator_create(size_t max_size);

void *
scas_linear_allocator_alloc(struct scas_linear_allocator_t *allocator, size_t size);

void
scas_linear_allocator_destroy(struct scas_linear_allocator_t *allocator);

#endif
