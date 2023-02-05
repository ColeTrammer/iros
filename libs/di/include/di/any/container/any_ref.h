#pragma once

#include <di/any/storage/ref_storage.h>
#include <di/any/vtable/inline_vtable.h>
#include <di/concepts/instance_of.h>
#include <di/meta/remove_cvref.h>
#include <di/types/prelude.h>
#include <di/util/voidify.h>

namespace di::any {
namespace detail {
    template<typename E, typename Interface>
    struct MethodImpl {};

    template<typename E, typename Tag, typename R, concepts::RemoveCVRefSameAs<This> Self, typename... BArgs,
             typename... Rest>
    struct MethodImpl<E, meta::List<Method<Tag, R(Self, BArgs...)>, Rest...>> : MethodImpl<E, meta::List<Rest...>> {
    private:
        constexpr static auto vtable(auto&& self) -> decltype(auto) { return (self.m_vtable); }
        constexpr static auto storage(auto&& self) -> decltype(auto) { return (self.m_storage); }

        constexpr friend R tag_invoke(Tag, meta::Like<Self, E> self, BArgs... bargs) {
            auto const& vtable = MethodImpl::vtable(self);
            auto* storage = util::voidify(util::addressof(MethodImpl::storage(self)));

            auto erased_fp = vtable[Method<Tag, R(Self, BArgs...)> {}];
            return erased_fp(storage, util::forward<BArgs>(bargs)...);
        }
    };
}

template<concepts::Interface Interface>
class AnyRef : private detail::MethodImpl<AnyRef<Interface>, meta::Transform<Interface, meta::Quote<meta::Type>>> {
private:
    template<typename, typename>
    friend struct detail::MethodImpl;

    using VTable = InlineVTable<Interface>;

public:
    AnyRef(AnyRef const&) = default;

    template<typename U>
    requires(concepts::ConstructibleFrom<RefStorage, InPlaceType<U>, U> && !concepts::RemoveCVRefSameAs<U, AnyRef> &&
             !concepts::InstanceOf<meta::RemoveCVRef<U>, InPlaceType>)
    constexpr AnyRef(U&& value)
        : m_vtable(VTable::template create_for<RefStorage, U>())
        , m_storage(in_place_type<U>, util::forward<U>(value)) {}

    AnyRef& operator=(AnyRef const&) = default;

private:
    InlineVTable<Interface> m_vtable;
    RefStorage m_storage;
};
}