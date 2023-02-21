#include <dius/prelude.h>

namespace dius {
static di::Result<void> sys_munmap(di::Byte* data, size_t length) {
    return system::system_call<int>(system::Number::munmap, data, length) % di::into_void;
}

MemoryRegion::~MemoryRegion() {
    if (!empty()) {
        (void) sys_munmap(data(), size());
    }
}
}
