#ifndef _SYS_PARAM_H
#define _SYS_PARAM_H 1

#include <signal.h>

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

#define ALIGN_UP(size, align) ((size_t)((size) == 0 ? 0 : ((align) == 0 ? (size) : ((((size) + (align) -1) / (align)) * (align)))))

#endif /* _SYS_PARAM_H */
