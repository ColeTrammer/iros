#pragma once

#include <di/any/concepts/vtable_for.h>
#include <di/concepts/convertible_to.h>
#include <di/concepts/object.h>
#include <di/concepts/reference.h>
#include <di/meta/list/prelude.h>
#include <di/types/prelude.h>
#include <di/util/addressof.h>

namespace di::any {
class RefStorage {
public:
    using Interface = meta::List<>;

    template<typename T>
    constexpr static bool creation_is_fallible(InPlaceType<T>) {
        return false;
    }

    constexpr RefStorage() : m_pointer(nullptr) {}

    template<concepts::Object T, concepts::ConvertibleTo<T&> U>
    requires(!concepts::Const<T>)
    constexpr RefStorage(InPlaceType<T&>, U&& u) : m_pointer(util::addressof(static_cast<T&>(util::forward<U>(u)))) {}

    template<concepts::Object T, concepts::ConvertibleTo<T const&> U>
    constexpr RefStorage(InPlaceType<T const&>, T const& u)
        : m_const_pointer(static_cast<T const&>(util::forward<U>(u))) {}

    template<concepts::LanguageFunction T, concepts::ConvertibleTo<T*> U>
    constexpr RefStorage(InPlaceType<T*>, T* u)
        : m_function_pointer(reinterpret_cast<void (*)()>(static_cast<T*>(util::forward<U>(u)))) {}

    ~RefStorage() = default;

    constexpr static void copy_construct(concepts::VTableFor<Interface> auto const&, RefStorage* dest,
                                         RefStorage const* source) {
        *dest = *source;
    }

    constexpr static void copy_assign(concepts::VTableFor<Interface> auto const&, RefStorage* dest,
                                      RefStorage const* source) {
        *dest = *source;
    }

    constexpr static void move_construct(concepts::VTableFor<Interface> auto const&, RefStorage* dest,
                                         RefStorage const* source) {
        *dest = *source;
    }

    constexpr static void move_assign(concepts::VTableFor<Interface> auto const&, RefStorage* dest,
                                      RefStorage const* source) {
        *dest = *source;
    }

    constexpr static void destroy(concepts::VTableFor<Interface> auto const&, RefStorage*) {}

    template<typename T>
    T* down_cast() const {
        if constexpr (concepts::Const<T>) {
            return static_cast<T*>(m_const_pointer);
        } else if constexpr (concepts::Object<T>) {
            return static_cast<T*>(m_pointer);
        } else {
            return reinterpret_cast<T*>(m_function_pointer);
        }
    }

private:
    RefStorage(RefStorage const&) = default;
    RefStorage& operator=(RefStorage const&) = default;

    union {
        void* m_pointer;
        void const* m_const_pointer;
        void (*m_function_pointer)();
    };
};
}