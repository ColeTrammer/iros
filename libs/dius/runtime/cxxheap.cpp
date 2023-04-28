#include <di/math/prelude.h>
#include <dius/print.h>
#include <dius/system/prelude.h>

[[gnu::weak]] void* operator new(std::size_t size) {
    return ::operator new(size, std::align_val_t { alignof(void*) });
}
[[gnu::weak]] void* operator new(std::size_t size, std::align_val_t alignment) {
    auto* result = ::operator new(size, alignment, std::nothrow);
    ASSERT(result);
    return result;
}

[[gnu::weak]] void* operator new(std::size_t size, std::nothrow_t const&) noexcept {
    return ::operator new(size, std::align_val_t { alignof(void*) }, std::nothrow);
}

[[gnu::weak]] void operator delete(void* pointer, std::nothrow_t const&) noexcept {
    return ::operator delete(pointer);
}
[[gnu::weak]] void operator delete(void* pointer, std::align_val_t alignment, std::nothrow_t const&) noexcept {
    return ::operator delete(pointer, alignment);
}

// Array allocating new.
[[nodiscard]] [[gnu::weak]] void* operator new[](std::size_t size) {
    return ::operator new(size);
}
[[nodiscard]] [[gnu::weak]] void* operator new[](std::size_t size, std::align_val_t alignment) {
    return ::operator new(size, alignment);
}
[[nodiscard]] [[gnu::weak]] void* operator new[](std::size_t size, std::nothrow_t const&) noexcept {
    return ::operator new(size, std::nothrow);
}
[[nodiscard]] [[gnu::weak]] void* operator new[](std::size_t size, std::align_val_t alignment,
                                                 std::nothrow_t const&) noexcept {
    return ::operator new(size, alignment, std::nothrow);
}

// Array deallocating delete.
[[gnu::weak]] void operator delete[](void* pointer) noexcept {
    return ::operator delete(pointer);
}
[[gnu::weak]] void operator delete[](void* pointer, std::size_t size) noexcept {
    return ::operator delete(pointer, size);
}
[[gnu::weak]] void operator delete[](void* pointer, std::align_val_t alignment) noexcept {
    return ::operator delete(pointer, alignment);
}
[[gnu::weak]] void operator delete[](void* pointer, std::size_t size, std::align_val_t alignment) noexcept {
    return ::operator delete(pointer, size, alignment);
}
[[gnu::weak]] void operator delete[](void* pointer, std::nothrow_t const&) noexcept {
    return ::operator delete(pointer, std::nothrow);
}
[[gnu::weak]] void operator delete[](void* pointer, std::align_val_t alignment, std::nothrow_t const&) noexcept {
    return ::operator delete(pointer, alignment, std::nothrow);
}

#ifdef DIUS_PLATFORM_LINUX
static uptr heap_end;
#endif

[[gnu::weak]] void* operator new(std::size_t size, std::align_val_t align, std::nothrow_t const&) noexcept {
#ifdef DIUS_PLATFORM_LINUX
    if (!heap_end) {
        heap_end = *dius::system::system_call<uptr>(dius::system::Number::brk, nullptr);
    }

    auto object_start = di::align_up(heap_end, di::to_underlying(align));
    ASSERT(object_start % di::to_underlying(align) == 0);

    auto new_heap_end = object_start + size;
    if (dius::system::system_call<uptr>(dius::system::Number::brk, new_heap_end) != new_heap_end) {
        dius::println("Failed to allocate memory of size {} with align {}"_sv, size, di::to_underlying(align));
        return nullptr;
    }

    heap_end = new_heap_end;
    return reinterpret_cast<void*>(object_start);
#else
    ASSERT_LT_EQ(di::to_underlying(align), 4096);

    auto result = dius::system::system_call<uptr>(dius::system::Number::allocate_memory, di::align_up(size, 4096));
    if (!result) {
        dius::println("Failed to allocate memory of size {} with align {}"_sv, size, di::to_underlying(align));
        return nullptr;
    }
    return reinterpret_cast<void*>(*result);
#endif
}

// Deallocating delete.
[[gnu::weak]] void operator delete(void*) noexcept {}
[[gnu::weak]] void operator delete(void*, std::size_t) noexcept {}
[[gnu::weak]] void operator delete(void*, std::align_val_t) noexcept {}
[[gnu::weak]] void operator delete(void*, std::size_t, std::align_val_t) noexcept {}
