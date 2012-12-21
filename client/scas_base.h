#ifndef SCAS_BASE_H
#define SCAS_BASE_H

#define UNUSED(x) (void)x

void
scas_log_init(void);

void
scas_log_shutdown(void);

void
scas_log(const char *format, ...);

#endif

