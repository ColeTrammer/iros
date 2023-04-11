#pragma once

#include <di/concepts/copyable.h>
#include <di/concepts/derived_from.h>
#include <di/concepts/same_as.h>
#include <di/container/iterator/iterator_category.h>
#include <di/container/iterator/iterator_ssize_type.h>
#include <di/container/iterator/iterator_value.h>
#include <di/container/types/prelude.h>
#include <di/types/prelude.h>
#include <di/util/declval.h>

namespace di::container {
template<typename Self, typename Category, typename ValueType, typename SSizeType>
struct IteratorBase {
private:
    constexpr Self& self() { return static_cast<Self&>(*this); }
    constexpr Self const& self() const { return static_cast<Self const&>(*this); }

public:
    IteratorBase() = default;
    IteratorBase(IteratorBase const&) = default;
    IteratorBase(IteratorBase&&) = default;

    IteratorBase& operator=(IteratorBase const&) = default;
    IteratorBase& operator=(IteratorBase&&) = default;

    IteratorBase(IteratorBase const&)
    requires(concepts::SameAs<Category, InputIteratorTag>)
    = delete;
    IteratorBase& operator=(IteratorBase const&)
    requires(concepts::SameAs<Category, InputIteratorTag>)
    = delete;

    constexpr Self& operator++() {
        self().advance_one();
        return self();
    }
    constexpr void operator++(int) { self().advance_one(); }

    constexpr Self operator++(int)
    requires(concepts::DerivedFrom<Category, ForwardIteratorTag>)
    {
        auto temp = self();
        self().advance_one();
        return temp;
    }

    constexpr Self& operator--()
    requires(concepts::DerivedFrom<Category, BidirectionalIteratorTag>)
    {
        self().back_one();
        return self();
    }

    constexpr Self operator--(int)
    requires(concepts::DerivedFrom<Category, BidirectionalIteratorTag>)
    {
        auto temp = self();
        self().back_one();
        return temp;
    }

    constexpr decltype(auto) operator[](SSizeType n) const
    requires(concepts::DerivedFrom<Category, RandomAccessIteratorTag>)
    {
        auto copy = self();
        copy.advance_n(n);
        return *copy;
    }

    constexpr Self& operator+=(SSizeType n)
    requires(concepts::DerivedFrom<Category, RandomAccessIteratorTag>)
    {
        self().advance_n(n);
        return self();
    }

    constexpr Self& operator-=(SSizeType n)
    requires(concepts::DerivedFrom<Category, RandomAccessIteratorTag>)
    {
        self().advance_n(-n);
        return self();
    }

private:
    constexpr friend Self operator+(Self const& self, SSizeType n)
    requires(concepts::DerivedFrom<Category, RandomAccessIteratorTag>)
    {
        auto temp = self;
        temp.advance_n(n);
        return temp;
    }

    constexpr friend Self operator+(SSizeType n, Self const& self)
    requires(concepts::DerivedFrom<Category, RandomAccessIteratorTag>)
    {
        auto temp = self;
        temp.advance_n(n);
        return temp;
    }

    constexpr friend Self operator-(Self const& self, SSizeType n)
    requires(concepts::DerivedFrom<Category, RandomAccessIteratorTag>)
    {
        auto temp = self;
        temp.advance_n(-n);
        return temp;
    }

    friend SSizeType tag_invoke(types::Tag<iterator_ssize_type>, InPlaceType<Self>) {
        return util::declval<SSizeType>();
    }
    friend InPlaceType<ValueType> tag_invoke(types::Tag<iterator_value>, InPlaceType<Self>) {
        return in_place_type<ValueType>;
    }
    friend Category tag_invoke(types::Tag<iterator_category>, InPlaceType<Self>) { return util::declval<Category>(); }
};
}
