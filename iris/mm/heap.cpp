#include <di/prelude.h>
#include <iris/core/log.h>

// These functions are explicitly not to be used in the iris kernel.
// Nothrow new and sized deallocations are required throughout the kernel.
// void* operator new(std::size_t size);
// void* operator new(std::size_t size, std::align_val_t alignment);

void* operator new(std::size_t size, std::nothrow_t const&) noexcept {
    return ::operator new(size, std::align_val_t { 16 }, std::nothrow);
}

void* operator new(std::size_t size, std::align_val_t alignment, std::nothrow_t const&) noexcept {
    iris::debug_log("Trying to allocate size={} alignment={}"_sv, size, di::to_underlying(alignment));
    return nullptr;
}

// Deallocating delete.
void operator delete(void*) noexcept {
    di::unreachable();
}
void operator delete(void*, std::size_t) noexcept {}
void operator delete(void*, std::align_val_t) noexcept {}
void operator delete(void*, std::size_t, std::align_val_t) noexcept {}