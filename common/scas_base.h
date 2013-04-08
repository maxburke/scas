/***********************************************************************
 * scas, Copyright (c) 2012-2013, Maximilian Burke
 * This file is distributed under the FreeBSD license. 
 * See LICENSE for details.
 ***********************************************************************/

#ifndef SCAS_BASE_H
#define SCAS_BASE_H

#include <stdint.h>
#include <stdio.h>
#include <signal.h>

#ifndef NDEBUG
#include <stdio.h>
#    define BREAK() do { fprintf(stderr, "%s:%d: Breakpoint raised\n", __FILE__, __LINE__); raise(SIGTRAP); } while (0)
#else
#    define BREAK() raise(SIGTRAP)
#endif

#define VERIFY(x) if (!(x)) { scas_log("%s:%d: %s", __FILE__, __LINE__, #x); BREAK(); } else (void)0

#define UNUSED(x) (void)x

#define ENOTIMPL 1
#define ENOCONN 2


struct scas_hash_t
{
    uint32_t hash[5];
};

void
scas_log_init(void);

void
scas_log_shutdown(void);

void
scas_log(const char *format, ...);

void
scas_log_system_error(const char *str);

void
scas_mkdir(const char *dir);

struct scas_hash_t
scas_hash_string(const char *string);

struct scas_hash_t
scas_hash_buffer(const void *ptr, size_t size);

#endif

