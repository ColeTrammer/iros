#include <di/function/prelude.h>
#include <dius/memory_region.h>
#include <dius/system/system_call.h>

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
