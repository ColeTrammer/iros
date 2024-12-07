#pragma once

#include "di/assert/assert_bool.h"
#include "di/container/allocator/allocator.h"
#include "di/container/allocator/fallible_allocator.h"
#include "di/container/allocator/infallible_allocator.h"
#include "di/platform/prelude.h"
#include "di/sync/atomic.h"
#include "di/util/immovable.h"
#include "di/util/std_new.h"
#include "di/vocab/error/prelude.h"
#include "di/vocab/expected/as_fallible.h"
#include "di/vocab/expected/try_infallible.h"
#include "di/vocab/expected/unexpected.h"
#include "di/vocab/pointer/intrusive_ptr.h"

namespace di::vocab {
struct ArcTag {};

template<typename T>
using Arc = IntrusivePtr<T, ArcTag>;

template<typename T>
struct IntrusiveRefCount : util::Immovable {
private:
    template<typename>
    friend struct MakeArcFunction;

public:
    template<typename = void>
    constexpr auto arc_from_this() {
        return Arc<T>(static_cast<T*>(this), adopt_object);
    }

protected:
    IntrusiveRefCount() = default;

private:
    constexpr friend void tag_invoke(types::Tag<intrusive_ptr_increment>, InPlaceType<ArcTag>, T* pointer) {
        auto* base = static_cast<IntrusiveRefCount*>(pointer);
        base->m_ref_count.fetch_add(1, sync::MemoryOrder::Relaxed);
    }

    constexpr friend void tag_invoke(types::Tag<intrusive_ptr_decrement>, InPlaceType<ArcTag>, T* pointer) {
        auto* base = static_cast<IntrusiveRefCount*>(pointer);
        if (base->m_ref_count.fetch_sub(1, sync::MemoryOrder::AcquireRelease) == 1) {
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

    sync::Atomic<usize> m_ref_count { 1 };
};

template<typename T>
struct MakeArcFunction {
    template<typename... Args>
    constexpr auto operator()(Args&&... args) const
    requires(requires { IntrusiveRefCount<T>::make(util::forward<Args>(args)...); })
    {
        return vocab::as_fallible(IntrusiveRefCount<T>::make(util::forward<Args>(args)...)) % [](T* pointer) {
            return Arc<T>(pointer, retain_object);
        } | vocab::try_infallible;
    }
};

template<detail::IntrusivePtrValid<ArcTag> T>
constexpr inline auto make_arc = MakeArcFunction<T> {};
}

namespace di {
using vocab::Arc;
using vocab::IntrusiveRefCount;
using vocab::make_arc;
}
