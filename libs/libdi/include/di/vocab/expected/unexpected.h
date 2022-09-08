#pragma once

#include <di/concepts/constructible_from.h>
#include <di/concepts/remove_cvref_same_as.h>
#include <di/types/in_place.h>
#include <di/util/forward.h>
#include <di/util/initializer_list.h>
#include <di/util/swap.h>

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
    constexpr explicit Unexpected(Args&&... args) : m_error(util::forward<Args>(args)...) {}

    template<typename T, typename... Args>
    requires(concepts::ConstructibleFrom<E, util::InitializerList<T>, Args...>)
    constexpr explicit Unexpected(util::InitializerList<T> list, Args&&... args) : m_error(list, util::forward<Args>(args)...) {}

    constexpr E& error() & { return m_error; }
    constexpr E const& error() const& { return m_error; }
    constexpr E&& error() && { return util::forward<E>(m_error); }
    constexpr E const&& error() const&& { return util::forward<E const>(m_error); }

private:
    constexpr friend void tag_invoke(types::Tag<util::swap>, Unexpected& a, Unexpected& b)
    requires(concepts::Swappable<E>)
    {
        util::swap(a.error(), b.error());
    }

    E m_error;
};

template<typename E>
Unexpected(E) -> Unexpected<E>;
}
