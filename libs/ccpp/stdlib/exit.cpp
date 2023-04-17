#include <dius/system/process.h>
#include <stdlib.h>

extern "C" void exit(int exit_code) {
    // FIXME: call atexit handlers and flush stdio.
    dius::system::exit_process(exit_code);
}
