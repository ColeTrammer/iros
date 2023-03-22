#pragma once

#include <di/types/size_t.h>
#ifdef DI_USE_STD
#include <new>
#else
namespace std {
enum class align_val_t : std::size_t {};

struct destroying_delete_t {
    explicit destroying_delete_t() = default;
};

constexpr inline auto destroying_delete = destroying_delete_t {};

struct nothrow_t {
    explicit nothrow_t() = default;
};

constexpr inline auto nothrow = nothrow_t {};

template<typename T>
[[nodiscard]] constexpr T* launder(T* pointer) noexcept {
    return __builtin_launder(pointer);
}
}

// Allocating new.
[[nodiscard]] void* operator new(std::size_t size);
[[nodiscard]] void* operator new(std::size_t size, std::align_val_t alignment);
[[nodiscard]] void* operator new(std::size_t size, std::nothrow_t const&) noexcept;
[[nodiscard]] void* operator new(std::size_t size, std::align_val_t alignment, std::nothrow_t const&) noexcept;

// Deallocating delete.
void operator delete(void* pointer) noexcept;
void operator delete(void* pointer, std::size_t size) noexcept;
void operator delete(void* pointer, std::align_val_t alignment) noexcept;
void operator delete(void* pointer, std::size_t size, std::align_val_t alignment) noexcept;
void operator delete(void* pointer, std::nothrow_t const&) noexcept;
void operator delete(void* pointer, std::align_val_t alignment, std::nothrow_t const&) noexcept;

// Array allocating new.
[[nodiscard]] void* operator new[](std::size_t size);
[[nodiscard]] void* operator new[](std::size_t size, std::align_val_t alignment);
[[nodiscard]] void* operator new[](std::size_t size, std::nothrow_t const&) noexcept;
[[nodiscard]] void* operator new[](std::size_t size, std::align_val_t alignment, std::nothrow_t const&) noexcept;

// Array deallocating delete.
void operator delete[](void* pointer) noexcept;
void operator delete[](void* pointer, std::size_t size) noexcept;
void operator delete[](void* pointer, std::align_val_t alignment) noexcept;
void operator delete[](void* pointer, std::size_t size, std::align_val_t alignment) noexcept;
void operator delete[](void* pointer, std::nothrow_t const&) noexcept;
void operator delete[](void* pointer, std::align_val_t alignment, std::nothrow_t const&) noexcept;

// Placement new.
[[nodiscard]] inline void* operator new(std::size_t, void* p) noexcept {
    return p;
}
[[nodiscard]] inline void* operator new[](std::size_t, void* p) noexcept {
    return p;
}

// Placement delete.
inline void operator delete(void*, void*) noexcept {};
inline void operator delete[](void*, void*) noexcept {};
#endif
