#pragma once

#include "di/container/algorithm/for_each.h"
#include "di/container/interface/size.h"
#include "di/container/meta/container_const_iterator.h"
#include "di/container/meta/container_iterator.h"
#include "di/container/view/zip.h"
#include "di/function/bind_back.h"
#include "di/meta/compare.h"
#include "di/meta/core.h"
#include "di/meta/operations.h"
#include "di/types/integers.h"
#include "di/util/add_member_get.h"
#include "di/util/forward.h"
#include "di/vocab/array/array.h"
#include "di/vocab/tuple/enable_generate_structed_bindings.h"

namespace di::math::linalg {
namespace detail {
    struct NoopMixin {};

    template<typename T>
    struct MixinHelper : meta::TypeConstant<NoopMixin> {};

    template<typename T>
    requires(requires { typename T::Mixin; })
    struct MixinHelper<T> : meta::TypeConstant<typename T::Mixin> {};

    template<typename T>
    using Mixin = meta::Type<MixinHelper<T>>;

    template<typename T>
    concept Tag = requires {
        typename T::Type;
        typename Constexpr<T::extent>;
    } && concepts::SameAs<decltype(T::extent), usize const>;

    template<Tag T>
    constexpr inline auto extent = T::extent;

    template<typename T>
    struct SignedTHelper : meta::TypeConstant<meta::MakeSigned<meta::Type<T>>> {};

    template<typename T>
    requires(requires { typename T::SignedT; })
    struct SignedTHelper<T> : meta::TypeConstant<typename T::SignedT> {};

    template<Tag T>
    using SignedT = meta::Type<SignedTHelper<T>>;
}

template<detail::Tag Tag>
class Vec
    : public detail::Mixin<Tag>
    , public util::AddMemberGet<Vec<Tag>> {
private:
    using T = meta::Type<Tag>;
    using SignedT = detail::SignedT<Tag>;
    constexpr static auto extent = detail::extent<Tag>;

    using Storage = di::Array<T, extent>;

public:
    Vec()
    requires(concepts::DefaultInitializable<T>)
    = default;

    template<concepts::ConvertibleTo<T>... Ts>
    requires(sizeof...(Ts) == extent)
    constexpr Vec(Ts&&... values) : m_values { T(di::forward<Ts>(values))... } {}

    constexpr auto values() -> Storage& { return m_values; }
    constexpr auto values() const -> Storage const& { return m_values; }

    constexpr auto begin() { return values().begin(); }
    constexpr auto begin() const { return values().begin(); }

    constexpr auto end() { return values().end(); }
    constexpr auto end() const { return values().end(); }

    constexpr auto operator+=(Vec const& other) -> Vec& {
        for (auto [a, b] : di::zip(*this, other)) {
            a += b;
        }
        return *this;
    }
    constexpr auto operator+=(T const& value) -> Vec& {
        for (auto& x : *this) {
            x += value;
        }
        return *this;
    }
    constexpr auto operator+=(SignedT const& value) -> Vec& requires(!di::SameAs<SignedT, T>) {
        for (auto& x : *this) {
            x += value;
        }
        return *this;
    }

    constexpr auto operator-=(Vec const& other) -> Vec& {
        for (auto [a, b] : di::zip(*this, other)) {
            a -= b;
        }
        return *this;
    }
    constexpr auto operator-=(T const& value) -> Vec& {
        for (auto& x : *this) {
            x -= value;
        }
        return *this;
    }
    constexpr auto operator-=(SignedT const& value) -> Vec& requires(!di::SameAs<SignedT, T>) {
        for (auto& x : *this) {
            x -= value;
        }
        return *this;
    }

    constexpr auto operator*=(T const& value) -> Vec& {
        for (auto& x : *this) {
            x *= value;
        }
        return *this;
    }

    constexpr auto operator/=(T const& value) -> Vec& {
        for (auto& x : *this) {
            x /= value;
        }
        return *this;
    }

    constexpr friend auto operator==(Vec const& a, Vec const& b) -> bool
    requires(concepts::EqualityComparable<T>)
    {
        return a.values() == b.values();
    }
    constexpr friend auto operator<=>(Vec const& a, Vec const& b)
    requires(concepts::ThreeWayComparable<T>)
    {
        return a.values() <=> b.values();
    }

private:
    constexpr friend auto operator+(Vec const& a, Vec const& b) -> Vec
    requires(concepts::Copyable<T>)
    {
        auto result = a;
        result += b;
        return result;
    }
    constexpr friend auto operator+(Vec const& a, T const& b) -> Vec
    requires(concepts::Copyable<T>)
    {
        auto result = a;
        result += b;
        return result;
    }
    constexpr friend auto operator+(Vec const& a, SignedT const& b) -> Vec
    requires(concepts::Copyable<T> && !di::SameAs<T, SignedT>)
    {
        auto result = a;
        result += b;
        return result;
    }
    constexpr friend auto operator+(T const& b, Vec const& a) -> Vec
    requires(concepts::Copyable<T>)
    {
        auto result = a;
        result += b;
        return result;
    }
    constexpr friend auto operator+(SignedT const& b, Vec const& a) -> Vec
    requires(concepts::Copyable<T> && !di::SameAs<T, SignedT>)
    {
        auto result = a;
        result += b;
        return result;
    }

    constexpr friend auto operator-(Vec const& a, Vec const& b) -> Vec
    requires(concepts::Copyable<T>)
    {
        auto result = a;
        result -= b;
        return result;
    }
    constexpr friend auto operator-(Vec const& a, T const& b) -> Vec
    requires(concepts::Copyable<T>)
    {
        auto result = a;
        result -= b;
        return result;
    }
    constexpr friend auto operator-(Vec const& a, SignedT const& b) -> Vec
    requires(concepts::Copyable<T> && !di::SameAs<T, SignedT>)
    {
        auto result = a;
        result -= b;
        return result;
    }
    constexpr friend auto operator-(T const& b, Vec const& a) -> Vec
    requires(concepts::Copyable<T>)
    {
        auto result = a;
        result -= b;
        return result;
    }
    constexpr friend auto operator-(SignedT const& b, Vec const& a) -> Vec
    requires(concepts::Copyable<T> && !di::SameAs<T, SignedT>)
    {
        auto result = a;
        result -= b;
        return result;
    }

    constexpr friend auto operator*(Vec const& a, T const& b) -> Vec {
        auto result = a;
        a *= b;
        return a;
    }
    constexpr friend auto operator*(T const& b, Vec& a) -> Vec {
        auto result = a;
        a *= b;
        return a;
    }

    constexpr friend auto operator/(Vec const& a, T const& b) -> Vec {
        auto result = a;
        a /= b;
        return a;
    }
    constexpr friend auto operator/(T const& b, Vec& a) -> Vec {
        auto result = a;
        a /= b;
        return a;
    }

    auto tag_invoke(di::Tag<di::size>, Vec const& self) { return self.values().size(); }

    constexpr friend auto tag_invoke(di::Tag<vocab::tuple_size>, di::InPlaceType<Vec>) { return extent; }

    template<usize index>
    requires(index < extent)
    constexpr friend auto tag_invoke(di::Tag<vocab::tuple_element>, di::InPlaceType<Vec>, Constexpr<index>)
        -> InPlaceType<T> {
        return {};
    }

    template<usize index>
    requires(index < extent)
    constexpr friend auto tag_invoke(di::Tag<vocab::tuple_element>, di::InPlaceType<Vec const>, Constexpr<index>)
        -> InPlaceType<T const> {
        return {};
    }

    template<concepts::DecaySameAs<Vec> Self, usize index>
    requires(index < extent)
    constexpr friend auto tag_invoke(di::Tag<util::get_in_place>, Constexpr<index>, Self&& self) -> decltype(auto) {
        return di::forward_like<Self>(self.values()[index]);
    }

    di::Array<T, extent> m_values {};
};
}
