#include <di/assert/assert_bool.h>
#include <di/math/prelude.h>
#include <di/util/unreachable.h>
#include <dius/print.h>
#include <dius/runtime/allocate.h>
#include <dius/system/prelude.h>

[[gnu::weak]] auto operator new(std::size_t size) -> void* {
    return ::operator new(size, std::align_val_t { alignof(void*) });
}
[[gnu::weak]] auto operator new(std::size_t size, std::align_val_t alignment) -> void* {
    auto* result = ::operator new(size, alignment, std::nothrow);
    ASSERT(result);
    return result;
}

[[gnu::weak]] auto operator new(std::size_t size, std::nothrow_t const&) noexcept -> void* {
    return ::operator new(size, std::align_val_t { alignof(void*) }, std::nothrow);
}

[[gnu::weak]] void operator delete(void* pointer, std::nothrow_t const&) noexcept {
    ::operator delete(pointer);
}
[[gnu::weak]] void operator delete(void* pointer, std::align_val_t alignment, std::nothrow_t const&) noexcept {
    ::operator delete(pointer, alignment);
}

// Array allocating new.
[[nodiscard]] [[gnu::weak]] auto operator new[](std::size_t size) -> void* {
    return ::operator new(size);
}
[[nodiscard]] [[gnu::weak]] auto operator new[](std::size_t size, std::align_val_t alignment) -> void* {
    return ::operator new(size, alignment);
}
[[nodiscard]] [[gnu::weak]] auto operator new[](std::size_t size, std::nothrow_t const&) noexcept -> void* {
    return ::operator new(size, std::nothrow);
}
[[nodiscard]] [[gnu::weak]] auto operator new[](std::size_t size, std::align_val_t alignment,
                                                std::nothrow_t const&) noexcept -> void* {
    return ::operator new(size, alignment, std::nothrow);
}

// Array deallocating delete.
[[gnu::weak]] void operator delete[](void* pointer) noexcept {
    ::operator delete(pointer);
}
[[gnu::weak]] void operator delete[](void* pointer, std::size_t size) noexcept {
    ::operator delete(pointer, size);
}
[[gnu::weak]] void operator delete[](void* pointer, std::align_val_t alignment) noexcept {
    ::operator delete(pointer, alignment);
}
[[gnu::weak]] void operator delete[](void* pointer, std::size_t size, std::align_val_t alignment) noexcept {
    ::operator delete(pointer, size, alignment);
}
[[gnu::weak]] void operator delete[](void* pointer, std::nothrow_t const&) noexcept {
    ::operator delete(pointer, std::nothrow);
}
[[gnu::weak]] void operator delete[](void* pointer, std::align_val_t alignment, std::nothrow_t const&) noexcept {
    ::operator delete(pointer, alignment, std::nothrow);
}

[[gnu::weak]] auto operator new(std::size_t size, std::align_val_t align, std::nothrow_t const&) noexcept -> void* {
    auto result = dius::runtime::Heap::the().allocate(size, usize(align));
    if (!result) {
        return nullptr;
    }
    return result.value().data;
}

// Deallocating delete.
[[gnu::weak]] void operator delete(void*) noexcept {
    // FIXME: Assert that this is never called, once coroutines have customer allocators which use sized deallocation.
}
[[gnu::weak]] void operator delete(void* pointer, std::size_t size) noexcept {
    ::operator delete(pointer, size, std::align_val_t { alignof(void*) });
}
[[gnu::weak]] void operator delete(void*, std::align_val_t) noexcept {
    // FIXME: Assert that this is never called, once coroutines have customer allocators which use sized deallocation.
}
[[gnu::weak]] void operator delete(void* pointer, std::size_t size, std::align_val_t align) noexcept {
    dius::runtime::Heap::the().deallocate(pointer, size, usize(align));
}
