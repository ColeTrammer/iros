#include <di/prelude.h>
#include <dius/prelude.h>

void* operator new(std::size_t size) {
    return ::operator new(size, std::align_val_t { alignof(void*) });
}
void* operator new(std::size_t size, std::align_val_t) {
    return ::operator new(size, std::align_val_t { alignof(void*) }, std::nothrow);
}

void* operator new(std::size_t size, std::nothrow_t const&) noexcept {
    return ::operator new(size, std::align_val_t { alignof(void*) }, std::nothrow);
}

static uptr heap_end;

void* operator new(std::size_t size, std::align_val_t align, std::nothrow_t const&) noexcept {
    if (!heap_end) {
        heap_end = *dius::system::system_call<uptr>(dius::system::Number::brk, nullptr);
    }

    heap_end = di::align_up(heap_end, di::to_underlying(align));
    (void) dius::system::system_call<uptr>(dius::system::Number::brk, heap_end + size);

    void* result = reinterpret_cast<void*>(heap_end);
    heap_end += size;
    return result;
}

// Deallocating delete.
void operator delete(void*) noexcept {}
void operator delete(void*, std::size_t) noexcept {}
void operator delete(void*, std::align_val_t) noexcept {}
void operator delete(void*, std::size_t, std::align_val_t) noexcept {}