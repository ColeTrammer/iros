#pragma once

#include "di/assert/assert_bool.h"
#include "di/container/allocator/allocator.h"
#include "di/container/allocator/fallible_allocator.h"
#include "di/container/allocator/infallible_allocator.h"
#include "di/util/immovable.h"
#include "di/util/std_new.h"
#include "di/vocab/error/prelude.h"
#include "di/vocab/expected/as_fallible.h"
#include "di/vocab/expected/try_infallible.h"
#include "di/vocab/expected/unexpected.h"
#include "di/vocab/pointer/intrusive_ptr.h"

namespace di::vocab {
struct RcTag {};

template<typename T>
using Rc = IntrusivePtr<T, RcTag>;

template<typename T>
struct IntrusiveThreadUnsafeRefCount : util::Immovable {
private:
    template<typename>
    friend struct MakeRcFunction;

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

    template<typename... Args, typename R = meta::AllocatorResult<platform::DefaultAllocator, T*>>
    constexpr static auto make(Args&&... args) -> R
    requires(requires { new (std::nothrow) T(util::forward<Args>(args)...); })
    {
        if consteval {
            return new T(util::forward<Args>(args)...);
        }
        auto* result = new (std::nothrow) T(util::forward<Args>(args)...);
        if constexpr (concepts::FallibleAllocator<platform::DefaultAllocator>) {
            if (!result) {
                return vocab::Unexpected(platform::BasicError::NotEnoughMemory);
            }
        } else {
            DI_ASSERT(result);
        }
        return result;
    }

    usize m_ref_count { 1 };
};

template<typename T>
struct MakeRcFunction {
    template<typename... Args>
    constexpr auto operator()(Args&&... args) const
    requires(requires { IntrusiveThreadUnsafeRefCount<T>::make(util::forward<Args>(args)...); })
    {
        return vocab::as_fallible(IntrusiveThreadUnsafeRefCount<T>::make(util::forward<Args>(args)...)) %
                   [](T* pointer) {
                       return Rc<T>(pointer, retain_object);
                   } |
               vocab::try_infallible;
    }
};

template<detail::IntrusivePtrValid<RcTag> T>
constexpr inline auto make_rc = MakeRcFunction<T> {};
}

namespace di {
using vocab::IntrusiveThreadUnsafeRefCount;
using vocab::make_rc;
using vocab::Rc;
}
