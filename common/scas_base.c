#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include <errno.h>

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

