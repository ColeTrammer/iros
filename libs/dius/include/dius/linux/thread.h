#pragma once

#include <dius/error.h>

#ifndef DIUS_USE_RUNTIME
#include <dius/posix/thread.h>
#else
namespace dius {
struct PlatformThread {
    int id() const { return 0; }
    di::Result<void> join();
};
}
#endif
