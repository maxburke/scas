/***********************************************************************
 * scas, Copyright (c) 2012-2013, Maximilian Burke
 * This file is distributed under the FreeBSD license. 
 * See LICENSE for details.
 ***********************************************************************/

#ifndef SCAS_ARG_PARSE_H
#define SCAS_ARG_PARSE_H

#include <stddef.h>

enum scas_arg_type_t
{
    /*
     * Switches have no other data to be parsed, they usually represent
     * boolean options, like whether or not to display a help message,
     * or a verbosity flag. 
     * For example, 
     *   foo.exe --verbose
     */
    ARG_TYPE_SWITCH,

    /*
     * Parameters are separated from the argument by whitespace and require
     * a value to be passed right after. Short arguments require that the
     * parameter abut the short argument, long arguments may have the 
     * argument separated by a space or equals sign. 
     * For example,
     *   foo.exe --context 2
     *   foo.exe --context=2
     *   foo.exe -C2
     *
     * Currently no error checking is done on the parameters, so if the user 
     * uses them incorrectly by, say, omitting the parameter (foo.exe --context -v)
     * no error is raised.
     */
    ARG_TYPE_PARAMETER,

    /*
     * Positional arguments are everything after the last named argument. For
     * example, these may be the list of files to compile with a compiler.
     * For example, 
     *   foo.exe --verbose bar.c baz.c
     */
    ARG_TYPE_POSITIONAL
};

struct scas_arg_t;

typedef void (*scas_arg_parse_callback_t)(void *context, const struct scas_arg_t *arg, const char *value);

struct scas_arg_t
{
    const char *short_form;
    const char *long_form;
    enum scas_arg_type_t type;
    scas_arg_parse_callback_t callback;
};

struct scas_arg_context_t
{
    int argc;
    char **argv;
    void *callback_context;
    size_t num_args;
    struct scas_arg_t *args;
    scas_arg_parse_callback_t positional_callback;
};

void
scas_arg_parse(struct scas_arg_context_t *context);

#endif

