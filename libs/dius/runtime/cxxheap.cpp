#include <di/prelude.h>
#include <dius/system/prelude.h>

void* operator new(std::size_t size) {
    return ::operator new(size, std::align_val_t { alignof(void*) });
}
void* operator new(std::size_t size, std::align_val_t alignment) {
    auto* result = ::operator new(size, alignment, std::nothrow);
    ASSERT(result);
    return result;
}

void* operator new(std::size_t size, std::nothrow_t const&) noexcept {
    return ::operator new(size, std::align_val_t { alignof(void*) }, std::nothrow);
}

#ifdef DIUS_PLATFORM_LINUX
static uptr heap_end;
#endif

void* operator new(std::size_t size, std::align_val_t align, std::nothrow_t const&) noexcept {
#ifdef DIUS_PLATFORM_LINUX
    if (!heap_end) {
        heap_end = *dius::system::system_call<uptr>(dius::system::Number::brk, nullptr);
    }

    auto new_heap_end = di::align_up(heap_end, di::to_underlying(align)) + size;
    if (dius::system::system_call<uptr>(dius::system::Number::brk, new_heap_end) != new_heap_end) {
        return nullptr;
    }

    void* result = reinterpret_cast<void*>(new_heap_end - size);
    heap_end = new_heap_end;
    return result;
#else
    ASSERT_LT_EQ(di::to_underlying(align), 4096);

    auto result = dius::system::system_call<uptr>(dius::system::Number::allocate_memory, di::align_up(size, 4096));
    if (!result) {
        return nullptr;
    }
    return reinterpret_cast<void*>(*result);
#endif
}

// Deallocating delete.
void operator delete(void*) noexcept {}
void operator delete(void*, std::size_t) noexcept {}
void operator delete(void*, std::align_val_t) noexcept {}
void operator delete(void*, std::size_t, std::align_val_t) noexcept {}
