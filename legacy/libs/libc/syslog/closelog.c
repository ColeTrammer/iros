#include <syslog.h>

#include "syslog_internal.h"

void closelog(void) {
    if (log_file) {
        fclose(log_file);
    }
}
