#include <signal.h>
#include <stddef.h>

sighandler_t signal(int sig, sighandler_t handler) {
    (void) sig;
    (void) handler;

    return NULL;
}