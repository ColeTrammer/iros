#pragma once

#include <liim/container/hash/forward.h>
#include <liim/tuple.h>
#include <liim/utilities.h>

namespace LIIM::Container::Hash {
template<typename T>
struct HashFunction {};

template<typename T>
using HashForType = HashFunction<decay_t<T>>;

template<typename T>
concept Hashable = requires(const T& value, Hasher& hasher) {
    HashForType<T>::hash(hasher, value);
};

template<typename TransparentKey, typename Base>
concept HashableLike = Hashable<Base> && Hashable<TransparentKey> && []() -> bool {
    if constexpr (SameAs<decay_t<Base>, decay_t<TransparentKey>>) {
        return true;
    }
    if constexpr (requires { typename HashForType<TransparentKey>::Matches; }) {
        auto helper = []<typename... Types>(Tuple<Types...>) {
            return TypeList::IsValid<decay_t<Base>, Types...> {};
        };
        return decltype(helper(declval<typename HashForType<TransparentKey>::Matches>()))::value;
    }
    return false;
}();
}
