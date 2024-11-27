#pragma once

#include <di/util/prelude.h>
#include <iris/core/error.h>
#include <iris/core/userspace_access.h>

namespace iris {
namespace detail {}
template<di::concepts::ImplicitLifetime T>
class UserspacePtr {
private:
    using Value = di::meta::RemoveConst<T>;

    union Uninit {
        Uninit() {}
        ~Uninit() {}

        Value value;
    };

    explicit UserspacePtr(T* pointer) : m_pointer(pointer) {}

public:
    static auto create(T* pointer) -> Expected<UserspacePtr<T>> {
        if (!validate_user_region(mm::VirtualAddress(di::to_uintptr(pointer)), 1, sizeof(T))) {
            return di::Unexpected(Error::BadAddress);
        }
        return UserspacePtr<T>(pointer);
    }

    auto read() const -> Expected<Value> {
        Uninit value;
        TRY(copy_from_user({ reinterpret_cast<byte const*>(m_pointer), sizeof(Value) },
                           reinterpret_cast<byte*>(&value.value)));
        return di::move(value.value);
    }

    auto write(Value const& value) const -> Expected<void> {
        return copy_to_user({ reinterpret_cast<byte const*>(&value), sizeof(Value) },
                            reinterpret_cast<byte*>(m_pointer));
    }

    auto raw_userspace_pointer() const -> T* { return m_pointer; }

private:
    T* m_pointer { nullptr };
};

template<typename T>
auto tag_invoke(di::Tag<di::util::deduce_create>, di::InPlaceTemplate<UserspacePtr>, T*) -> UserspacePtr<T>;
}
