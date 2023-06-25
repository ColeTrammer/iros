#pragma once

#include <di/meta/core.h>
#include <di/meta/language.h>
#include <di/meta/operations.h>
#include <di/meta/util.h>
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

namespace di {
using util::forward_as_base;
}
