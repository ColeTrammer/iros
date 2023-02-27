#pragma once

#include <di/any/concepts/prelude.h>
#include <di/any/meta/prelude.h>
#include <di/any/storage/prelude.h>
#include <di/any/types/prelude.h>
#include <di/any/vtable/prelude.h>
#include <di/function/monad/monad_try.h>
#include <di/util/addressof.h>
#include <di/util/forward.h>
#include <di/util/initializer_list.h>
#include <di/util/swap.h>
#include <di/util/voidify.h>

namespace di::any {
namespace detail {
    template<typename E, typename S, typename Interface>
    struct MethodImpl {};

    template<typename E, typename S, typename Tag, typename R, concepts::RemoveCVRefSameAs<This> Self,
             typename... BArgs, typename... Rest>
    struct MethodImpl<E, S, meta::List<Method<Tag, R(Self, BArgs...)>, Rest...>>
        : MethodImpl<E, S, meta::List<Rest...>> {
    private:
        constexpr static auto vtable(auto&& self) -> decltype(auto) { return (self.m_vtable); }
        constexpr static auto storage(E& self) -> S& { return self; }
        constexpr static auto storage(E const& self) -> S const& { return self; }

        constexpr friend R tag_invoke(Tag, meta::Like<Self, E> self, BArgs... bargs) {
            auto const& vtable = MethodImpl::vtable(self);
            auto* storage = util::voidify(util::addressof(MethodImpl::storage(self)));

            auto erased_fp = vtable[Method<Tag, R(Self, BArgs...)> {}];
            return erased_fp(storage, util::forward<BArgs>(bargs)...);
        }
    };
}

template<concepts::Interface UserInterface, concepts::AnyStorage Storage = HybridStorage<>,
         typename VTablePolicy = MaybeInlineVTable<3>>
class Any
    : private detail::MethodImpl<Any<UserInterface, Storage, VTablePolicy>, Storage,
                                 meta::MergeInterfaces<UserInterface, typename Storage::Interface>>
    , public Storage {
    template<typename, typename>
    friend struct detail::MethodImpl;

private:
    using Interface = meta::MergeInterfaces<UserInterface, typename Storage::Interface>;
    using VTable = meta::Invoke<VTablePolicy, Interface>;

    static_assert(concepts::VTableFor<VTable, Interface>,
                  "VTablePolicy policy returned an invalid VTable for the provided interface.");

    constexpr static auto storage_category = Storage::storage_category();

    constexpr static bool is_reference = storage_category == StorageCategory::Reference;
    constexpr static bool is_trivially_destructible = is_reference || storage_category == StorageCategory::Trivial;
    constexpr static bool is_trivially_copyable = is_reference || storage_category == StorageCategory::Trivial;
    constexpr static bool is_trivially_moveable = is_trivially_copyable;

    constexpr static bool is_moveable = !is_trivially_moveable && storage_category != StorageCategory::Immovable;
    constexpr static bool is_copyable = storage_category == StorageCategory::Copyable;

    template<typename T>
    using RemoveConstructQualifiers = meta::Conditional<is_reference, T, meta::RemoveCVRef<T>>;

public:
    template<typename U, concepts::Impl<Interface> VU = RemoveConstructQualifiers<U>>
    requires(!concepts::RemoveCVRefSameAs<U, Any> && !concepts::InstanceOf<meta::RemoveCVRef<U>, InPlaceType> &&
             concepts::AnyStorable<VU, Storage> && concepts::ConstructibleFrom<VU, U>)
    constexpr static Any create(U&& value) {
        if constexpr (!concepts::AnyStorableInfallibly<VU, Storage>) {
            auto result = Any {};
            DI_ASSERT(Storage::init(util::addressof(result), in_place_type<VU>, util::forward<U>(value)));
            result.m_vtable = VTable::template create_for<Storage, VU>();
            return result;
        } else {
            return Any(in_place_type<VU>, util::forward<U>(value));
        }
    }

    template<typename T, typename... Args, concepts::Impl<Interface> VT = RemoveConstructQualifiers<T>>
    requires(concepts::AnyStorable<VT, Storage> && concepts::ConstructibleFrom<VT, Args...>)
    constexpr static Any create(InPlaceType<T>, Args&&... args) {
        if constexpr (!concepts::AnyStorableInfallibly<VT, Storage>) {
            auto result = Any {};
            DI_ASSERT(Storage::init(util::addressof(result), in_place_type<VT>, util::forward<Args>(args)...));
            result.m_vtable = VTable::template create_for<Storage, VT>();
            return result;
        } else {
            return Any(in_place_type<VT>, util::forward<Args>(args)...);
        }
    }

    template<typename T, typename U, typename... Args, concepts::Impl<Interface> VT = RemoveConstructQualifiers<T>>
    requires(concepts::AnyStorable<VT, Storage> && concepts::ConstructibleFrom<VT, util::InitializerList<U>&, Args...>)
    constexpr static Any create(InPlaceType<T>, util::InitializerList<U> list, Args&&... args) {
        if constexpr (!concepts::AnyStorableInfallibly<VT, Storage>) {
            auto result = Any {};
            DI_ASSERT(Storage::init(util::addressof(result), in_place_type<VT>, list, util::forward<Args>(args)...));
            result.m_vtable = VTable::template create_for<Storage, VT>();
            return result;
        } else {
            return Any(in_place_type<VT>, list, util::forward<Args>(args)...);
        }
    }

    template<typename U, concepts::Impl<Interface> VU = RemoveConstructQualifiers<U>>
    requires(!concepts::RemoveCVRefSameAs<U, Any> && !concepts::InstanceOf<meta::RemoveCVRef<U>, InPlaceType> &&
             concepts::AnyStorable<VU, Storage> && concepts::ConstructibleFrom<VU, U>)
    constexpr static auto try_create(U&& value) {
        if constexpr (!concepts::AnyStorableInfallibly<VU, Storage>) {
            auto result = Any {};
            return Storage::init(util::addressof(result), in_place_type<VU>, util::forward<U>(value)) % [&] {
                result.m_vtable = VTable::template create_for<Storage, VU>();
                return result;
            };
        } else {
            return Any(in_place_type<VU>, util::forward<U>(value));
        }
    }

    template<typename T, typename... Args, concepts::Impl<Interface> VT = RemoveConstructQualifiers<T>>
    requires(concepts::AnyStorable<VT, Storage> && concepts::ConstructibleFrom<VT, Args...>)
    constexpr static auto try_create(InPlaceType<T>, Args&&... args) {
        if constexpr (!concepts::AnyStorableInfallibly<VT, Storage>) {
            auto result = Any {};
            return Storage::init(util::addressof(result), in_place_type<VT>, util::forward<Args>(args)...) % [&] {
                result.m_vtable = VTable::template create_for<Storage, VT>();
                return result;
            };
        } else {
            return Any(in_place_type<VT>, util::forward<Args>(args)...);
        }
    }

    template<typename T, typename U, typename... Args, concepts::Impl<Interface> VT = RemoveConstructQualifiers<T>>
    requires(concepts::AnyStorable<VT, Storage> && concepts::ConstructibleFrom<VT, util::InitializerList<U>&, Args...>)
    constexpr static Result<Any> try_create(InPlaceType<T>, util::InitializerList<U> list, Args&&... args) {
        if constexpr (!concepts::AnyStorableInfallibly<VT, Storage>) {
            auto result = Any {};
            return Storage::init(util::addressof(result), in_place_type<VT>, list, util::forward<Args>(args)...) % [&] {
                result.m_vtable = VTable::template create_for<Storage, VT>();
                return result;
            };
        } else {
            return Any(in_place_type<VT>, list, util::forward<Args>(args)...);
        }
    }

    Any()
    requires(!is_reference)
    = default;

    Any(Any const&)
    requires(is_trivially_copyable)
    = default;

    Any(Any const& other)
    requires(is_copyable)
        : Storage(), m_vtable(other.m_vtable) {
        Storage::copy_construct(other.m_vtable, this, util::addressof(other));
    }

    Any(Any&&)
    requires(is_trivially_moveable)
    = default;

    Any(Any&& other)
    requires(is_moveable)
        : m_vtable(other.m_vtable) {
        Storage::move_construct(other.m_vtable, this, util::addressof(other));
    }

    template<typename U, concepts::Impl<Interface> VU = RemoveConstructQualifiers<U>>
    requires(!concepts::RemoveCVRefSameAs<U, Any> && !concepts::InstanceOf<meta::RemoveCVRef<U>, InPlaceType> &&
             concepts::AnyStorableInfallibly<VU, Storage> && concepts::ConstructibleFrom<VU, U>)
    constexpr Any(U&& value) : m_vtable(VTable::template create_for<Storage, VU>()) {
        Storage::init(this, in_place_type<VU>, util::forward<U>(value));
    }

    template<typename T, typename... Args, concepts::Impl<Interface> VT = RemoveConstructQualifiers<T>>
    requires(concepts::AnyStorableInfallibly<VT, Storage> && concepts::ConstructibleFrom<VT, Args...>)
    constexpr Any(InPlaceType<T>, Args&&... args) : m_vtable(VTable::template create_for<Storage, VT>()) {
        Storage::init(this, in_place_type<VT>, util::forward<Args>(args)...);
    }

    template<typename T, typename U, typename... Args, concepts::Impl<Interface> VT = RemoveConstructQualifiers<T>>
    requires(concepts::AnyStorableInfallibly<VT, Storage> &&
             concepts::ConstructibleFrom<VT, util::InitializerList<U>&, Args...>)
    constexpr Any(InPlaceType<T>, util::InitializerList<U> list, Args&&... args)
        : m_vtable(VTable::template create_for<Storage, VT>()) {
        Storage::init(this, in_place_type<VT>, list, util::forward<Args>(args)...);
    }

    ~Any()
    requires(is_trivially_destructible)
    = default;

    ~Any()
    requires(!is_trivially_destructible)
    {
        Storage::destroy(m_vtable, this);
    }

    Any& operator=(Any const&)
    requires(is_trivially_copyable)
    = default;

    Any& operator=(Any&&)
    requires(is_trivially_moveable)
    = default;

    Any& operator=(Any const& other)
    requires(is_copyable)
    {
        Storage::copy_assign(m_vtable, this, other.m_vtable, util::addressof(other));
        return *this;
    }

    Any& operator=(Any&& other)
    requires(is_moveable)
    {
        Storage::move_assign(m_vtable, this, other.m_vtable, util::addressof(other));
        return *this;
    }

    template<typename U, concepts::Impl<Interface> VU = RemoveConstructQualifiers<U>>
    requires(!concepts::RemoveCVRefSameAs<U, Any> && !concepts::InstanceOf<meta::RemoveCVRef<U>, InPlaceType> &&
             concepts::AnyStorableInfallibly<VU, Storage> && concepts::ConstructibleFrom<VU, U>)
    Any& operator=(U&& value) {
        if (!is_trivially_destructible) {
            Storage::destroy(m_vtable, this);
        }
        m_vtable = VTable::template create_for<Storage, VU>();
        Storage::init(this, in_place_type<VU>, util::forward<U>(value));
        return *this;
    }

    constexpr explicit operator bool() const
    requires(!is_reference)
    {
        return !m_vtable.empty();
    }

    constexpr bool empty() const
    requires(!is_reference)
    {
        return !*this;
    }

    void reset()
    requires(!is_reference)
    {
        if (!is_trivially_destructible) {
            Storage::destroy(m_vtable, this);
        }
        m_vtable.reset();
    }

private:
    VTable m_vtable {};
};
}
