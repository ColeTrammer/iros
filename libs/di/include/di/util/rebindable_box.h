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
    struct IsRebindableBoxHelper : meta::FalseType {};

    template<typename T>
    struct IsRebindableBoxHelper<RebindableBox<T>> : meta::TrueType {};

    template<typename T>
    concept IsRebindableBox = IsRebindableBoxHelper<meta::RemoveCVRef<T>>::value;
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
    requires(concepts::ConstructibleFrom<Storage, U const&> && detail::RebindableBoxCanConvertConstructor<T, U>)
    constexpr explicit(!concepts::ConvertibleTo<U const&, Storage>) RebindableBox(RebindableBox<U> const& other)
        : m_storage(other.value()) {}

    template<typename U>
    requires(concepts::ConstructibleFrom<Storage, U> && detail::RebindableBoxCanConvertConstructor<T, U>)
    constexpr explicit(!concepts::ConvertibleTo<U, Storage>) RebindableBox(RebindableBox<U>&& other)
        : m_storage(util::move(other).value()) {}

    template<typename U = T>
    requires(concepts::ConstructibleFrom<Storage, U> && !concepts::RemoveCVRefSameAs<RebindableBox, U> &&
             !concepts::RemoveCVRefSameAs<types::InPlace, U>)
    constexpr explicit(!concepts::ConvertibleTo<U, Storage>) RebindableBox(U&& value)
        : m_storage(util::forward<U>(value)) {}

    template<typename... Args>
    requires(concepts::ConstructibleFrom<Storage, Args...>)
    constexpr RebindableBox(types::InPlace, Args&&... args) : m_storage(util::forward<Args>(args)...) {}

    template<typename U, typename... Args>
    requires(concepts::ConstructibleFrom<Storage, std::initializer_list<U>, Args...>)
    constexpr RebindableBox(types::InPlace, std::initializer_list<U> list, Args&&... args)
        : m_storage(list, util::forward<Args>(args)...) {}

    constexpr ~RebindableBox() = default;

    constexpr RebindableBox& operator=(RebindableBox const&) = default;
    constexpr RebindableBox& operator=(RebindableBox&&) = default;

    constexpr RebindableBox& operator=(RebindableBox const& other)
    requires(!concepts::TriviallyCopyAssignable<Storage> && concepts::CopyConstructible<Storage>)
    {
        rebind(other.value());
        return *this;
    }

    constexpr RebindableBox& operator=(RebindableBox const& other)
    requires(!concepts::TriviallyMoveAssignable<Storage> && concepts::MoveConstructible<Storage>)
    {
        rebind(util::move(other).value());
        return *this;
    }

    template<typename U>
    requires(concepts::ConstructibleFrom<Storage, U const&>)
    constexpr RebindableBox& operator=(RebindableBox<U> const& other) {
        rebind(other.value());
        return *this;
    }

    template<typename U>
    requires(concepts::ConstructibleFrom<Storage, U>)
    constexpr RebindableBox& operator=(RebindableBox<U>&& other) {
        rebind(util::move(other).value());
        return *this;
    }

    template<typename U = T>
    requires(!concepts::RemoveCVRefSameAs<U, RebindableBox> && !detail::IsRebindableBox<U> &&
             concepts::ConstructibleFrom<Storage, U>)
    constexpr RebindableBox& operator=(U&& value) {
        rebind(util::forward<U>(value));
        return *this;
    }

    constexpr T& value() & { return m_storage; }
    constexpr T const& value() const& { return m_storage; }
    constexpr T&& value() && { return util::move(m_storage); }
    constexpr T const&& value() const&& { return util::move(m_storage); }

    template<typename... Args>
    requires(concepts::ConstructibleFrom<T, Args...>)
    constexpr T& emplace(Args&&... args) {
        util::destroy_at(util::addressof(m_storage));
        util::construct_at(util::addressof(m_storage), util::forward<Args>(args)...);
        return value();
    }

    template<typename U, typename... Args>
    requires(concepts::ConstructibleFrom<T, std::initializer_list<U>, Args...>)
    constexpr T& emplace(std::initializer_list<U> list, Args&&... args) {
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
