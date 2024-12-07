#pragma once

#include "di/meta/core.h"
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
// Rebindable box is a wrapper around a type T or a reference T&,
// which always has rebind semantics on assignment. In particular,
// RebindableBox<T&>::operator= will change not assign-through to
// the object referenced. In addition, the assignment operator is
// defined even if the underlying T does not support it, as long
// as it is constructible from what is being assigned to it. Thus,
// a RebindableBox can rebind structures which contain references.
template<typename T>
class RebindableBox;

namespace detail {
    template<typename T, typename U>
    concept RebindableBoxCanConvertConstructor =
        (!concepts::ConstructibleFrom<T, RebindableBox<U>> && !concepts::ConstructibleFrom<T, RebindableBox<U> const> &&
         !concepts::ConstructibleFrom<T, RebindableBox<U>&> &&
         !concepts::ConstructibleFrom<T, RebindableBox<U> const&> && !concepts::ConvertibleTo<RebindableBox<U>, T> &&
         !concepts::ConvertibleTo<RebindableBox<U> const, T> && !concepts::ConvertibleTo<RebindableBox<U>&, T> &&
         !concepts::ConvertibleTo<RebindableBox<U> const&, T>);

    template<typename T>
    concept IsRebindableBox = concepts::InstanceOf<meta::RemoveCVRef<T>, RebindableBox>;
}

template<typename T>
class RebindableBox {
private:
    using Storage = meta::WrapReference<T>;

public:
    constexpr RebindableBox()
    requires(concepts::DefaultConstructible<Storage>)
    = default;

    constexpr RebindableBox(RebindableBox const&)
    requires(!concepts::CopyConstructible<Storage>)
    = delete;
    constexpr RebindableBox(RebindableBox const&) = default;

    constexpr RebindableBox(RebindableBox&&)
    requires(concepts::MoveConstructible<Storage>)
    = default;

    template<typename U>
    requires(!concepts::RemoveCVRefSameAs<T, U> && concepts::ConstructibleFrom<Storage, U const&> &&
             detail::RebindableBoxCanConvertConstructor<T, U>)
    constexpr explicit(!concepts::ConvertibleTo<U const&, Storage>) RebindableBox(RebindableBox<U> const& other)
        : m_storage(other.value()) {}

    template<typename U>
    requires(!concepts::RemoveCVRefSameAs<T, U> && concepts::ConstructibleFrom<Storage, U> &&
             detail::RebindableBoxCanConvertConstructor<T, U>)
    constexpr explicit(!concepts::ConvertibleTo<U, Storage>) RebindableBox(RebindableBox<U>&& other)
        : m_storage(util::move(other).value()) {}

    template<typename U>
    requires(!concepts::RemoveCVRefSameAs<U, RebindableBox> && concepts::ConstructibleFrom<Storage, U> &&
             !concepts::RemoveCVRefSameAs<RebindableBox, U> && !concepts::RemoveCVRefSameAs<types::InPlace, U>)
    constexpr explicit RebindableBox(U&& value) : m_storage(util::forward<U>(value)) {}

    template<typename... Args>
    requires(concepts::ConstructibleFrom<Storage, Args...>)
    constexpr RebindableBox(types::InPlace, Args&&... args) : m_storage(util::forward<Args>(args)...) {}

    template<typename U, typename... Args>
    requires(concepts::ConstructibleFrom<Storage, std::initializer_list<U>, Args...>)
    constexpr RebindableBox(types::InPlace, std::initializer_list<U> list, Args&&... args)
        : m_storage(list, util::forward<Args>(args)...) {}

    constexpr ~RebindableBox() = default;

    constexpr auto operator=(RebindableBox const&) -> RebindableBox& = default;
    constexpr auto operator=(RebindableBox&&) -> RebindableBox& = default;

    constexpr auto operator=(RebindableBox const& other)
        -> RebindableBox& requires(!concepts::TriviallyCopyAssignable<Storage> &&
                                   concepts::CopyConstructible<Storage>) {
            rebind(other.value());
            return *this;
        }

    constexpr auto operator=(RebindableBox const& other)
        -> RebindableBox& requires(!concepts::TriviallyMoveAssignable<Storage> &&
                                   concepts::MoveConstructible<Storage>) {
            rebind(util::move(other).value());
            return *this;
        }

    template<typename U>
    requires(concepts::ConstructibleFrom<Storage, U const&>)
    constexpr auto operator=(RebindableBox<U> const& other) -> RebindableBox& {
        rebind(other.value());
        return *this;
    }

    template<typename U>
    requires(concepts::ConstructibleFrom<Storage, U>)
    constexpr auto operator=(RebindableBox<U>&& other) -> RebindableBox& {
        rebind(util::move(other).value());
        return *this;
    }

    template<typename U = T>
    requires(!concepts::RemoveCVRefSameAs<U, RebindableBox> && !detail::IsRebindableBox<U> &&
             concepts::ConstructibleFrom<Storage, U>)
    constexpr auto operator=(U&& value) -> RebindableBox& {
        rebind(util::forward<U>(value));
        return *this;
    }

    constexpr auto value() & -> T& { return m_storage; }
    constexpr auto value() const& -> T const& { return m_storage; }
    constexpr auto value() && -> T&& { return util::move(m_storage); }
    constexpr auto value() const&& -> T const&& { return util::move(m_storage); }

    template<typename... Args>
    requires(concepts::ConstructibleFrom<T, Args...>)
    constexpr auto emplace(Args&&... args) -> T& {
        util::destroy_at(util::addressof(m_storage));
        util::construct_at(util::addressof(m_storage), util::forward<Args>(args)...);
        return value();
    }

    template<typename U, typename... Args>
    requires(concepts::ConstructibleFrom<T, std::initializer_list<U>, Args...>)
    constexpr auto emplace(std::initializer_list<U> list, Args&&... args) -> T& {
        util::destroy_at(util::addressof(m_storage));
        util::construct_at(util::addressof(m_storage), list, util::forward<Args>(args)...);
        return value();
    }

private:
    template<typename U>
    constexpr void rebind(U&& new_value) {
        constexpr bool const_assignable = concepts::AssignableFrom<T const&, U>;
        constexpr bool assignable = concepts::AssignableFrom<T&, U>;

        // Any type T which is const-assignable, must be, by definition, a reference
        // or a proxy reference. Therefore, it must be rebound. Also, rebinding is
        // required if the type is out-right not assignable.
        if constexpr (const_assignable || !assignable) {
            util::destroy_at(util::addressof(this->value()));
            util::construct_at(util::addressof(m_storage), util::forward<U>(new_value));
        } else {
            m_storage = util::forward<U>(new_value);
        }
    }

    Storage m_storage {};
};

template<typename T>
RebindableBox(T&&) -> RebindableBox<meta::UnwrapRefDecay<T>>;

template<typename T>
constexpr auto make_rebindable_box(T&& value) {
    return RebindableBox<meta::UnwrapRefDecay<T>> { util::forward<T>(value) };
}
}

namespace di {
using util::RebindableBox;
}
