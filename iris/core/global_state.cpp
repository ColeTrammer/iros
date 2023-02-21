#include <iris/core/global_state.h>

namespace iris {
// Wrapper type which disables construction. This is needed to prevent GCC
// from trying to destruct the global state, which requires __cxa_atexit
// and __dso_handle. GCC inists on calling the destructor even when the
// global state is placed in a union, since it must be trivally destructible.
// https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p1247r0.html
template<class T>
class DisableDestruction {
    alignas(T) di::Byte m_data[sizeof(T)];

public:
    template<class... Args>
    requires(di::concepts::ConstructibleFrom<T, Args...>)
    explicit DisableDestruction(Args&&... args) {
        ::new (di::voidify(m_data)) T(di::forward<Args>(args)...);
    }

    T& get() { return *reinterpret_cast<T*>(m_data); }
};

static DisableDestruction<iris::GlobalState> s_global_state;

GlobalState& global_state_in_boot() {
    return s_global_state.get();
}
}
