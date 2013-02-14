#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/stat.h>

#include "scas_base.h"

static FILE *log;

void
scas_log_init(void)
{
    if (!log)
    {
        log = fopen("scas_client.log", "w");
    }
}

void
scas_log_shutdown(void)
{
    if (log)
    {
        fclose(log);
    }
}

void
scas_log(const char *format, ...)
{
    FILE *fp;
    va_list args;

    fp = (!log) ? stderr : log;

    va_start(args, format);
    fprintf(fp, "[scas] ");
    vfprintf(fp, format, args);
    fprintf(fp, "\n");
    va_end(args);
}

void
scas_log_system_error(const char *str)
{
    int error;
    char error_buf[256];

    error = errno;
    strerror_r(error, error_buf, sizeof error_buf);
    error_buf[255] = 0;

    scas_log("Error: %s. (%d: %s)", str, error, error_buf);

    BREAK();
}

void
scas_mkdir(const char *dir)
{
    int result;

    result = mkdir(dir, S_IRUSR | S_IWUSR);

    if (result != 0)
    {
        if (errno == EEXIST)
        {
            struct stat meta;

            result = stat(dir, &meta);
            assert(result == 0);

            /*
             * If the directory already exists then we're good. If it doesn't,
             * and instead it is actually a file, then we have a problem. So,
             * abort!
             */
            if (S_ISDIR(meta.st_mode))
            {
                return;
            }

            scas_log_system_error("Regular file with name exists where you were trying to create a directory.");
        }

        scas_log_system_error("Could not create directory.");
        abort();
    }
}

