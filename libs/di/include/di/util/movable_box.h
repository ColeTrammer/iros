#pragma once

#include "di/meta/operations.h"
#include "di/meta/trivial.h"
#include "di/meta/util.h"
#include "di/meta/vocab.h"
#include "di/types/in_place.h"
#include "di/util/addressof.h"
#include "di/util/construct_at.h"
#include "di/util/destroy_at.h"
#include "di/util/forward.h"
#include "di/util/initializer_list.h"
#include "di/util/move.h"
#include "di/util/swap.h"

namespace di::util {
/// MovableBox takes a non-movable default constructible type T
/// and allows it to be move constructed, by defaulting the new T.
///
/// This is intended to be used by objects which internally use a
/// mutex or atomic variable, but need to be movable so that they can
/// be passed through an Expected<>.
template<concepts::DefaultConstructible T>
requires(!concepts::MoveConstructible<T>)
class MovableBox {
public:
    MovableBox() = default;

    MovableBox(MovableBox const&) = delete;
    constexpr MovableBox(MovableBox&&) : MovableBox() {}

    auto operator=(MovableBox const&) -> MovableBox& = delete;
    auto operator=(MovableBox&&) -> MovableBox& = delete;

    template<typename... Args>
    requires(concepts::ConstructibleFrom<T, Args...>)
    constexpr explicit MovableBox(types::InPlace, Args&&... args) : m_value(util::forward<Args>(args)...) {}

    constexpr auto value() & -> T& { return m_value; }
    constexpr auto value() const& -> T const& { return m_value; }
    constexpr auto value() && -> T&& { return util::move(m_value); }
    constexpr auto value() const&& -> T const&& { return util::move(m_value); }

private:
    T m_value {};
};
}

namespace di {
using util::MovableBox;
}
