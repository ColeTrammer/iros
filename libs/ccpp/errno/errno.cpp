#include <errno.h>

extern "C" {
__thread constinit int errno = 0;
}
