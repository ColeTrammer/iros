#pragma once

#include <di/util/immovable.h>
#include <di/util/std_new.h>
#include <di/vocab/error/prelude.h>
#include <di/vocab/pointer/intrusive_ptr.h>

namespace di::vocab {
struct RcTag {};

template<typename T>
using Rc = IntrusivePtr<T, RcTag>;

template<typename T>
struct IntrusiveThreadUnsafeRefCount : util::Immovable {
private:
    template<typename>
    friend struct MakeRcFunction;

    template<typename>
    friend struct TryMakeRcFunction;

public:
    template<typename = void>
    constexpr auto rc_from_this() {
        return Rc<T>(static_cast<T*>(this), adopt_object);
    }

protected:
    IntrusiveThreadUnsafeRefCount() = default;

private:
    constexpr friend void tag_invoke(types::Tag<intrusive_ptr_increment>, InPlaceType<RcTag>, T* pointer) {
        auto* base = static_cast<IntrusiveThreadUnsafeRefCount*>(pointer);
        ++base->m_ref_count;
    }

    constexpr friend void tag_invoke(types::Tag<intrusive_ptr_decrement>, InPlaceType<RcTag>, T* pointer) {
        auto* base = static_cast<IntrusiveThreadUnsafeRefCount*>(pointer);
        if (base->m_ref_count-- == 1) {
            delete pointer;
        }
    }

    template<typename... Args,
             typename E = meta::ExpectedError<decltype(platform::DefaultFallibleAllocator<T>().allocate(1))>,
             typename R = Expected<T*, E>>
    constexpr static R make(Args&&... args)
    requires(requires { T(util::forward<Args>(args)...); })
    {
        if consteval {
            return new T(util::forward<Args>(args)...);
        } else {
            return platform::DefaultFallibleAllocator<T>().allocate(1) % [&](container::Allocation<T> result) {
                new (result.data) T(util::forward<Args>(args)...);
                return result.data;
            };
        }
    }

    usize m_ref_count { 1 };
};

template<typename T>
struct MakeRcFunction {
    template<typename... Args>
    constexpr Rc<T> operator()(Args&&... args) const
    requires(requires { IntrusiveThreadUnsafeRefCount<T>::make(util::forward<Args>(args)...); })
    {
        auto result = IntrusiveThreadUnsafeRefCount<T>::make(util::forward<Args>(args)...);
        DI_ASSERT(result);
        return Rc<T>(*result, retain_object);
    }
};

template<typename T>
struct TryMakeRcFunction {
    template<typename... Args>
    constexpr auto operator()(Args&&... args) const
    requires(requires { IntrusiveThreadUnsafeRefCount<T>::make(util::forward<Args>(args)...); })
    {
        return IntrusiveThreadUnsafeRefCount<T>::make(util::forward<Args>(args)...) % [](auto* pointer) {
            return Rc<T>(pointer, retain_object);
        };
    }
};

template<detail::IntrusivePtrValid<RcTag> T>
constexpr inline auto make_rc = MakeRcFunction<T> {};

template<detail::IntrusivePtrValid<RcTag> T>
constexpr inline auto try_make_rc = TryMakeRcFunction<T> {};
}
