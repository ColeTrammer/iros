#pragma once

#include <di/prelude.h>
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

public:
    explicit UserspacePtr(T* pointer) : m_pointer(pointer) {}

    Expected<Value> read() {
        Uninit value;
        TRY(copy_from_user({ reinterpret_cast<byte const*>(m_pointer), sizeof(Value) },
                           reinterpret_cast<byte*>(&value.value)));
        return di::move(value.value);
    }

    Expected<void> write(Value const& value) {
        return copy_to_user({ reinterpret_cast<byte const*>(&value), sizeof(Value) }, m_pointer);
    }

    T* raw_userspace_pointer() { return m_pointer; }

private:
    T* m_pointer { nullptr };
};
}
