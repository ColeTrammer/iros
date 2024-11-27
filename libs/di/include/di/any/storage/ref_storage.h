#pragma once

#include <di/any/concepts/vtable_for.h>
#include <di/any/storage/storage_category.h>
#include <di/meta/algorithm.h>
#include <di/meta/language.h>
#include <di/meta/operations.h>
#include <di/types/prelude.h>
#include <di/util/addressof.h>

namespace di::any {
class RefStorage {
public:
    using Interface = meta::List<>;

    constexpr static auto storage_category() -> StorageCategory { return StorageCategory::Reference; }

    template<typename T>
    constexpr static auto creation_is_fallible(InPlaceType<T>) -> bool {
        return false;
    }

    template<typename>
    using CreationResult = void;

    constexpr RefStorage() : m_pointer(nullptr) {}

    RefStorage(RefStorage const&) = default;
    auto operator=(RefStorage const&) -> RefStorage& = default;

    template<concepts::Object T, concepts::ConvertibleTo<T&> U>
    requires(!concepts::Const<T>)
    constexpr static void init(RefStorage* self, InPlaceType<T&>, U&& u) {
        self->m_pointer = util::addressof(static_cast<T&>(util::forward<U>(u)));
    }

    template<concepts::Object T, concepts::ConvertibleTo<T const&> U>
    constexpr static void init(RefStorage* self, InPlaceType<T const&>, T const& u) {
        self->m_const_pointer = util::addressof(static_cast<T const&>(util::forward<U>(u)));
    }

    template<concepts::LanguageFunction T, concepts::ConvertibleTo<T*> U>
    constexpr static void init(RefStorage* self, InPlaceType<T*>, T* u) {
        self->m_function_pointer = reinterpret_cast<void (*)()>(static_cast<T*>(util::forward<U>(u)));
    }

    ~RefStorage() = default;

    template<typename T>
    constexpr auto down_cast() const -> T* {
        if constexpr (concepts::Const<T>) {
            return static_cast<T*>(m_const_pointer);
        } else if constexpr (concepts::Object<T>) {
            return static_cast<T*>(m_pointer);
        } else {
            return reinterpret_cast<T*>(m_function_pointer);
        }
    }

private:
    union {
        void* m_pointer;
        void const* m_const_pointer;
        void (*m_function_pointer)();
    };
};
}
