#include <netdb.h>
#include <stddef.h>

static struct hostent static_hostent;
static char static_hostent_buffer[1024];

struct hostent *gethostent(void) {
    struct hostent *result = NULL;
    gethostent_r(&static_hostent, static_hostent_buffer, sizeof(static_hostent_buffer), &result, &h_errno);
    return result;
}
