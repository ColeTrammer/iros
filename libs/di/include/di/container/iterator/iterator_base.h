#pragma once

#include "di/container/iterator/iterator_category.h"
#include "di/container/iterator/iterator_ssize_type.h"
#include "di/container/iterator/iterator_value.h"
#include "di/container/types/prelude.h"
#include "di/meta/core.h"
#include "di/meta/operations.h"
#include "di/types/prelude.h"
#include "di/util/declval.h"

namespace di::container {
template<typename Self, typename Category, typename ValueType, typename SSizeType>
struct IteratorBase {
private:
    constexpr auto self() -> Self& { return static_cast<Self&>(*this); }
    constexpr auto self() const -> Self const& { return static_cast<Self const&>(*this); }

public:
    IteratorBase() = default;
    IteratorBase(IteratorBase const&) = default;
    IteratorBase(IteratorBase&&) = default;

    auto operator=(IteratorBase const&) -> IteratorBase& = default;
    auto operator=(IteratorBase&&) -> IteratorBase& = default;

    IteratorBase(IteratorBase const&)
    requires(concepts::SameAs<Category, InputIteratorTag>)
    = delete;
    auto operator=(IteratorBase const&)
        -> IteratorBase& requires(concepts::SameAs<Category, InputIteratorTag>) = delete;

    constexpr auto operator++() -> Self& {
        self().advance_one();
        return self();
    }
    constexpr void operator++(int) { self().advance_one(); }

    constexpr auto operator++(int) -> Self
    requires(concepts::DerivedFrom<Category, ForwardIteratorTag>)
    {
        auto temp = self();
        self().advance_one();
        return temp;
    }

    constexpr auto operator--() -> Self& requires(concepts::DerivedFrom<Category, BidirectionalIteratorTag>) {
        self().back_one();
        return self();
    }

    constexpr auto operator--(int) -> Self
    requires(concepts::DerivedFrom<Category, BidirectionalIteratorTag>)
    {
        auto temp = self();
        self().back_one();
        return temp;
    }

    constexpr auto operator[](SSizeType n) const -> decltype(auto)
    requires(concepts::DerivedFrom<Category, RandomAccessIteratorTag>)
    {
        auto copy = self();
        copy.advance_n(n);
        return *copy;
    }

constexpr auto operator+=(SSizeType n) -> Self& requires(concepts::DerivedFrom<Category, RandomAccessIteratorTag>) {
    self().advance_n(n);
    return self();
}

constexpr auto operator-=(SSizeType n) -> Self& requires(concepts::DerivedFrom<Category, RandomAccessIteratorTag>) {
    self().advance_n(-n);
    return self();
}

private
    : constexpr friend auto operator+(Self const& self, SSizeType n) -> Self
      requires(concepts::DerivedFrom<Category, RandomAccessIteratorTag>)
{
        auto temp = self;
        temp.advance_n(n);
        return temp;
    }

    constexpr friend auto operator+(SSizeType n, Self const& self) -> Self
    requires(concepts::DerivedFrom<Category, RandomAccessIteratorTag>)
    {
        auto temp = self;
        temp.advance_n(n);
        return temp;
    }

    constexpr friend auto operator-(Self const& self, SSizeType n) -> Self
    requires(concepts::DerivedFrom<Category, RandomAccessIteratorTag>)
    {
        auto temp = self;
        temp.advance_n(-n);
        return temp;
    }

    friend auto tag_invoke(types::Tag<iterator_ssize_type>, InPlaceType<Self>) -> SSizeType {
        return util::declval<SSizeType>();
    }
    friend auto tag_invoke(types::Tag<iterator_value>, InPlaceType<Self>) -> InPlaceType<ValueType> {
        return in_place_type<ValueType>;
    }
    friend auto tag_invoke(types::Tag<iterator_category>, InPlaceType<Self>) -> Category {
        return util::declval<Category>();
    }
};
}
