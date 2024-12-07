#pragma once

#include "di/meta/operations.h"
#include "di/meta/util.h"
#include "di/types/in_place.h"
#include "di/util/forward.h"
#include "di/util/initializer_list.h"
#include "di/util/move.h"
#include "di/util/rebindable_box.h"
#include "di/util/swap.h"

namespace di::vocab {
template<typename E>
class Unexpected {
public:
    constexpr Unexpected(Unexpected const&) = default;
    constexpr Unexpected(Unexpected&&) = default;

    template<typename T = E>
    requires(!concepts::RemoveCVRefSameAs<T, Unexpected> && !concepts::RemoveCVRefSameAs<T, types::InPlace> &&
             concepts::ConstructibleFrom<E, T>)
    constexpr explicit Unexpected(T&& error) : m_error(util::forward<T>(error)) {}

    template<typename... Args>
    requires(concepts::ConstructibleFrom<E, Args...>)
    constexpr explicit Unexpected(types::InPlace, Args&&... args)
        : m_error(types::in_place, util::forward<Args>(args)...) {}

    template<typename T, typename... Args>
    requires(concepts::ConstructibleFrom<E, std::initializer_list<T>, Args...>)
    constexpr explicit Unexpected(types::InPlace, std::initializer_list<T> list, Args&&... args)
        : m_error(types::in_place, list, util::forward<Args>(args)...) {}

    constexpr auto error() & -> E& { return m_error.value(); }
    constexpr auto error() const& -> E const& { return m_error.value(); }
    constexpr auto error() && -> E&& { return util::move(m_error).value(); }
    constexpr auto error() const&& -> E const&& { return util::move(m_error).value(); }

private:
    constexpr friend void tag_invoke(types::Tag<util::swap>, Unexpected& a, Unexpected& b)
    requires(concepts::Swappable<E>)
    {
        util::swap(a.m_error, b.m_error);
    }

    util::RebindableBox<E> m_error;
};

template<typename E>
Unexpected(E&&) -> Unexpected<meta::UnwrapRefDecay<E>>;
}
