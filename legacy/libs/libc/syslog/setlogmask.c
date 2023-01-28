#include <syslog.h>

#include "syslog_internal.h"

int setlogmask(int mask) {
    int ret = log_mask;
    log_mask = mask;
    return ret;
}
