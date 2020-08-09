#ifndef _SYSLOG_SYSLOG_INTERNAL_H
#define _SYSLOG_SYSLOG_INTERNAL_H 1

#include <stdio.h>

#define PRIVATE __attribute__((visibility("internal")))

extern PRIVATE int log_options;
extern PRIVATE int log_facility;
extern PRIVATE int log_mask;
extern PRIVATE const char *log_identity;
extern PRIVATE FILE *log_file;

void do_openlog(void) PRIVATE;

#endif /* _SYSLOG_SYSLOG_INTERNAL_H */
