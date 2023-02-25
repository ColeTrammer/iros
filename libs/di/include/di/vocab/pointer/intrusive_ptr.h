#pragma once

#include <di/types/prelude.h>
#include <di/util/exchange.h>
#include <di/vocab/pointer/intrusive_ptr_cpo.h>

#if __GNUC__ >= 12
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wuse-after-free"
#pragma GCC diagnostic ignored "-Wfree-nonheap-object"
#endif

namespace di::vocab {
struct RetainObject {
    RetainObject() = default;
};
constexpr inline auto retain_object = RetainObject {};

struct AdoptObject {
    AdoptObject() = default;
};
constexpr inline auto adopt_object = AdoptObject {};

template<typename T, typename Tag>
class IntrusivePtr {
public:
    constexpr IntrusivePtr() : m_pointer(nullptr) {}
    constexpr IntrusivePtr(nullptr_t) : IntrusivePtr() {}

    constexpr IntrusivePtr(IntrusivePtr const& other) { reset(other.get(), adopt_object); }
    constexpr IntrusivePtr(IntrusivePtr&& other) : m_pointer(other.release()) {}

    constexpr IntrusivePtr(T* pointer, RetainObject) : m_pointer(pointer) {
        static_assert(detail::IntrusivePtrValid<T, Tag>, "Invalid intrusive pointer Tag type.");
    }
    constexpr IntrusivePtr(T* pointer, AdoptObject) {
        static_assert(detail::IntrusivePtrValid<T, Tag>, "Invalid intrusive pointer Tag type.");
        reset(pointer, adopt_object);
    }

    constexpr ~IntrusivePtr() { reset(); }

    constexpr IntrusivePtr& operator=(nullptr_t) {
        reset();
        return *this;
    }
    constexpr IntrusivePtr& operator=(IntrusivePtr const& other) {
        reset(other.get(), adopt_object);
        return *this;
    }
    constexpr IntrusivePtr& operator=(IntrusivePtr&& other) {
        reset();
        m_pointer = other.release();
        return *this;
    }

    constexpr explicit operator bool() const { return !!get(); }

    constexpr T& operator*() const { return *get(); }
    constexpr T* operator->() const { return get(); }
    constexpr T* get() const { return m_pointer; }

    [[nodiscard]] constexpr T* release() { return util::exchange(m_pointer, nullptr); }

    constexpr void reset() {
        auto* old = release();
        if (old) {
            intrusive_ptr_decrement(in_place_type<Tag>, old);
        }
    }
    constexpr void reset(T* pointer, RetainObject) {
        reset();
        m_pointer = pointer;
    }
    constexpr void reset(T* pointer, AdoptObject) {
        reset();
        m_pointer = pointer;
        if (*this) {
            intrusive_ptr_increment(in_place_type<Tag>, get());
        }
    }

private:
    constexpr friend bool operator==(IntrusivePtr const& a, IntrusivePtr const& b) { return a.get() == b.get(); }
    constexpr friend di::strong_ordering operator<=>(IntrusivePtr const& a, IntrusivePtr const& b) {
        return a.get() <=> b.get();
    }

    T* m_pointer { nullptr };
};
}

#if __GNUC__ >= 12
#pragma GCC diagnostic pop
#endif
