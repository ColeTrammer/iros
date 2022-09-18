#pragma once

#include <di/concepts/copyable.h>
#include <di/concepts/same_as.h>
#include <di/container/iterator/iterator_ssize_type.h>
#include <di/container/iterator/iterator_value.h>
#include <di/types/prelude.h>
#include <di/util/declval.h>

namespace di::container {
namespace detail {
    template<typename T>
    concept Input = requires(T& iterator, T const& citerator) {
                        *citerator;
                        iterator.advance_one();
                    };

    template<typename T>
    concept Forward = Input<T> && concepts::Copyable<T>;

    template<typename T>
    concept Bidirectional = Forward<T> && requires(T& iterator) { iterator.back_one(); };

    template<typename T, typename SSizeType>
    concept RandomAccess = Bidirectional<T> && requires(T& iterator, SSizeType n) { iterator.advance_n(n); };
}

template<typename Self, typename ValueType, typename SSizeType>
struct IteratorBase {
private:
    constexpr Self& self() { return static_cast<Self&>(*this); }
    constexpr Self const& self() const { return static_cast<Self const&>(*this); }

public:
    constexpr Self& operator++() {
        self().advance_one();
        return self();
    }
    constexpr void operator++(int) { self().advance_one(); }

    constexpr Self operator++(int)
    requires(detail::Forward<Self>)
    {
        auto temp = self();
        self().advance_one();
        return temp;
    }

    constexpr Self& operator--()
    requires(detail::Bidirectional<Self>)
    {
        self().back_one();
        return self();
    }

    constexpr Self operator--(int)
    requires(detail::Bidirectional<Self>)
    {
        auto temp = self();
        self().back_one();
        return temp;
    }

    constexpr decltype(auto) operator[](SSizeType n) const
    requires(detail::RandomAccess<Self, SSizeType>)
    {
        auto copy = self();
        copy.advance_n(n);
        return *copy;
    }

    constexpr Self& operator+=(SSizeType n)
    requires(detail::RandomAccess<Self, SSizeType>)
    {
        self().advance_n(n);
        return self();
    }

    constexpr Self& operator-=(SSizeType n)
    requires(detail::RandomAccess<Self, SSizeType>)
    {
        self().advance_n(-n);
        return self();
    }

private:
    constexpr friend Self operator+(Self const& self, SSizeType n)
    requires(detail::RandomAccess<Self, SSizeType>)
    {
        auto temp = self;
        self.advance_n(n);
        return temp;
    }

    constexpr friend Self operator+(SSizeType n, Self const& self)
    requires(detail::RandomAccess<Self, SSizeType>)
    {
        auto temp = self;
        self.advance_n(n);
        return temp;
    }

    constexpr friend Self operator-(Self const& self, SSizeType n)
    requires(detail::RandomAccess<Self, SSizeType>)
    {
        auto temp = self;
        self.advance_n(-n);
        return temp;
    }

    friend SSizeType tag_invoke(types::Tag<iterator_ssize_type>, InPlaceType<Self>) { return util::declval<SSizeType>(); }
    friend ValueType tag_invoke(types::Tag<iterator_value>, InPlaceType<Self>) { return util::declval<ValueType>(); }
};
}