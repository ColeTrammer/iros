#pragma once

#include <di/container/algorithm/compare.h>
#include <di/container/algorithm/equal.h>
#include <di/container/ring/ring_operations.h>

namespace di::container {
template<typename Self, typename Value>
class ConstantRingInterface {
private:
    constexpr Self& self() { return static_cast<Self&>(*this); }
    constexpr Self const& self() const { return static_cast<Self const&>(*this); }

public:
    constexpr usize size() const { return ring::size(self()); }
    constexpr usize size_bytes() const { return ring::size_bytes(self()); }
    [[nodiscard]] constexpr bool empty() const { return ring::empty(self()); }

    constexpr auto begin() { return ring::begin(self()); }
    constexpr auto end() { return ring::end(self()); }

    constexpr auto begin() const { return ring::begin(self()); }
    constexpr auto end() const { return ring::end(self()); }

    constexpr auto front() { return ring::front(self()); }
    constexpr auto front() const { return ring::front(self()); }

    constexpr auto back() { return ring::back(self()); }
    constexpr auto back() const { return ring::back(self()); }

    constexpr auto at(usize index) { return ring::at(self(), index); }
    constexpr auto at(usize index) const { return ring::at(self(), index); }

    constexpr decltype(auto) operator[](usize index) { return ring::lookup(self(), index); }
    constexpr decltype(auto) operator[](usize index) const { return ring::lookup(self(), index); }

    constexpr auto iterator(usize index) { return ring::iterator(self(), index); }
    constexpr auto iterator(usize index) const { return ring::iterator(self(), index); }
    constexpr auto citerator(usize index) const { return ring::iterator(self(), index); }

private:
    constexpr friend bool operator==(Self const& a, Self const& b)
    requires(concepts::EqualityComparable<Value>)
    {
        return container::equal(a, b);
    }

    constexpr friend auto operator<=>(Self const& a, Self const& b)
    requires(concepts::ThreeWayComparable<Value>)
    {
        return container::compare(a, b);
    }
};
}
