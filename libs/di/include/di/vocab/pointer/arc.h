#pragma once

#include <di/sync/atomic.h>
#include <di/util/immovable.h>
#include <di/util/std_new.h>
#include <di/vocab/error/prelude.h>
#include <di/vocab/pointer/intrusive_ptr.h>

namespace di::vocab {
struct ArcTag {};

template<detail::IntrusivePtrValid<ArcTag> T>
using Arc = IntrusivePtr<T, ArcTag>;

template<typename T>
struct IntrusiveRefCount : util::Immovable {
private:
    template<typename>
    friend struct MakeArcFunction;

    template<typename>
    friend struct TryMakeArcFunction;

public:
    template<typename = void>
    auto arc_from_this() {
        return Arc<T>(static_cast<T*>(this), adopt_object);
    }

protected:
    IntrusiveRefCount() = default;

private:
    friend void tag_invoke(types::Tag<intrusive_ptr_increment>, InPlaceType<ArcTag>, T* pointer) {
        auto* base = static_cast<IntrusiveRefCount*>(pointer);
        base->m_ref_count.fetch_add(1, sync::MemoryOrder::Relaxed);
    }

    friend void tag_invoke(types::Tag<intrusive_ptr_decrement>, InPlaceType<ArcTag>, T* pointer) {
        auto* base = static_cast<IntrusiveRefCount*>(pointer);
        if (base->m_ref_count.fetch_sub(1, sync::MemoryOrder::AcquireRelease) == 1) {
            delete pointer;
        }
    }

    template<typename... Args>
    constexpr static T* make(Args&&... args)
    requires(requires { T(util::forward<Args>(args)...); })
    {
        if consteval {
            return new T(util::forward<Args>(args)...);
        } else {
            return ::new (std::nothrow) T(util::forward<Args>(args)...);
        }
    }

    sync::Atomic<usize> m_ref_count { 1 };
};

template<typename T>
struct MakeArcFunction {
    template<typename... Args>
    constexpr Arc<T> operator()(Args&&... args) const
    requires(requires { IntrusiveRefCount<T>::make(util::forward<Args>(args)...); })
    {
        auto* pointer = IntrusiveRefCount<T>::make(util::forward<Args>(args)...);
        DI_ASSERT(pointer);
        return Arc<T>(pointer, retain_object);
    }
};

template<typename T>
struct TryMakeArcFunction {
    template<typename... Args>
    constexpr Result<Arc<T>> operator()(Args&&... args) const
    requires(requires { IntrusiveRefCount<T>::make(util::forward<Args>(args)...); })
    {
        auto* pointer = IntrusiveRefCount<T>::make(util::forward<Args>(args)...);
        if (!pointer) {
            return Unexpected(BasicError::FailedAllocation);
        }
        return Arc<T>(pointer, retain_object);
    }
};

template<detail::IntrusivePtrValid<ArcTag> T>
constexpr inline auto make_arc = MakeArcFunction<T> {};

template<detail::IntrusivePtrValid<ArcTag> T>
constexpr inline auto try_make_arc = TryMakeArcFunction<T> {};
}