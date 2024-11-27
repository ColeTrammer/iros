#include <dius/memory_region.h>

#include <di/function/prelude.h>
#include <dius/system/system_call.h>

namespace dius {
static auto sys_munmap(di::Byte* data, size_t length) -> di::Result<void> {
    return system::system_call<int>(system::Number::munmap, data, length) % di::into_void;
}

MemoryRegion::~MemoryRegion() {
    if (!empty()) {
        (void) sys_munmap(data(), size());
    }
}
}
