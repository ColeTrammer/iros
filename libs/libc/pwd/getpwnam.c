#include <pwd.h>
#include <stddef.h>

extern struct passwd __static_passwd_buffer;
extern char __static_passwd_string_buffer[1024];

struct passwd *getpwnam(const char *name) {
    struct passwd *result;
    getpwnam_r(name, &__static_passwd_buffer, __static_passwd_string_buffer, sizeof(__static_passwd_string_buffer), &result);
    return result;
}
