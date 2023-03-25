#pragma once

#include <dius/error.h>

namespace dius {
struct PlatformThread {
    int id() const { return 0; }
    di::Result<void> join();
};
}
