#include <dius/memory_region.h>

#include <sys/mman.h>

namespace dius {
MemoryRegion::~MemoryRegion() {
    if (!empty()) {
        ::munmap(data(), size());
    }
}
}