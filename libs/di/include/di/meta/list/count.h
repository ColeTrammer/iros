#pragma once

#include <di/concepts/same_as.h>
#include <di/meta/list/concepts/type_list.h>
#include <di/meta/list/list.h>
#include <di/meta/size_constant.h>

namespace di::meta {
namespace detail {
    template<typename T, typename List>
    struct CountHelper;

    template<typename T>
    struct CountHelper<T, List<>> : meta::SizeConstant<0> {};

    template<typename T, typename U, typename... Rest>
    struct CountHelper<T, List<U, Rest...>> {
        constexpr static size_t value = +concepts::SameAs<T, U> + CountHelper<T, List<Rest...>>::value;
    };
}

template<concepts::TypeList List, typename T>
constexpr static size_t Count = detail::CountHelper<T, List>::value;
}
