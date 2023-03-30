#pragma once

#include <di/concepts/const.h>
#include <di/concepts/derived_from.h>
#include <di/concepts/reference.h>
#include <di/meta/like.h>
#include <di/meta/remove_cvref.h>
#include <di/meta/type_identity.h>
#include <di/util/forward_like.h>

namespace di::util {
template<typename Derived, typename Base>
requires(concepts::DerivedFrom<meta::RemoveCVRef<Derived>, Base>)
constexpr meta::Like<meta::RemoveReference<Derived>&, Base>&& forward_as_base(meta::RemoveReference<Derived>& derived) {
    if constexpr (concepts::Const<meta::RemoveReference<Derived>>) {
        return forward_like<meta::RemoveReference<Derived>&>(static_cast<Base const&>(derived));
    } else {
        return forward_like<meta::RemoveReference<Derived>&>(static_cast<Base&>(derived));
    }
}

template<typename Derived, typename Base>
requires(concepts::DerivedFrom<meta::RemoveCVRef<Derived>, Base>)
constexpr meta::Like<meta::RemoveReference<Derived>&&, Base>&&
forward_as_base(meta::RemoveReference<Derived>&& derived) {
    if constexpr (concepts::Const<meta::RemoveReference<Derived>>) {
        return forward_like<meta::RemoveReference<Derived>&&>(static_cast<Base const&>(derived));
    } else {
        return forward_like<meta::RemoveReference<Derived>&&>(static_cast<Base&>(derived));
    }
}
}
