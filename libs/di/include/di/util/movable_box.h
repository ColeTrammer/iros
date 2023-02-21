#pragma once

#include <di/concepts/assignable_from.h>
#include <di/concepts/constructible_from.h>
#include <di/concepts/convertible_to.h>
#include <di/concepts/copy_constructible.h>
#include <di/concepts/default_constructible.h>
#include <di/concepts/move_assignable.h>
#include <di/concepts/move_constructible.h>
#include <di/concepts/remove_cvref_same_as.h>
#include <di/concepts/trivially_copy_assignable.h>
#include <di/concepts/trivially_move_assignable.h>
#include <di/meta/false_type.h>
#include <di/meta/true_type.h>
#include <di/meta/unwrap_ref_decay.h>
#include <di/meta/wrap_reference.h>
#include <di/types/in_place.h>
#include <di/util/addressof.h>
#include <di/util/construct_at.h>
#include <di/util/destroy_at.h>
#include <di/util/forward.h>
#include <di/util/initializer_list.h>
#include <di/util/move.h>
#include <di/util/swap.h>

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

    MovableBox& operator=(MovableBox const&) = delete;
    MovableBox& operator=(MovableBox&&) = delete;

    template<typename... Args>
    requires(concepts::ConstructibleFrom<T, Args...>)
    constexpr explicit MovableBox(types::InPlace, Args&&... args) : m_value(util::forward<Args>(args)...) {}

    T& value() & { return m_value; }
    T const& value() const& { return m_value; }
    T&& value() && { return util::move(m_value); }
    T const&& value() const&& { return util::move(m_value); }

private:
    T m_value {};
};
}
