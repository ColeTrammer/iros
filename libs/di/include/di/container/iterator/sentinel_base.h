#pragma once

namespace di::container {
namespace detail {
    template<typename It, typename T>
    concept Sized = requires(It const& it, T const& sent) { sent.difference(it); };

    template<typename It, typename T>
    concept Equal = requires(It const& it, T const& sent) { sent.equals(it); };
}

template<typename Self>
class SentinelBase {
private:
    template<detail::Sized<Self> Iter>
    constexpr friend auto operator-(Self const& sent, Iter const& it) {
        return sent.difference(it);
    }

    template<detail::Sized<Self> Iter>
    constexpr friend auto operator-(Iter const& it, Self const& sent) {
        return -sent.difference(it);
    }

    template<typename U>
    requires(detail::Equal<U, Self>)
    constexpr friend bool operator==(Self const& sent, U const& it) {
        return sent.equals(it);
    }
};
}
