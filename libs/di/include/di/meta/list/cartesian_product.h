#pragma once

#include <di/meta/list/bind_back.h>
#include <di/meta/list/list.h>
#include <di/meta/list/push_front.h>
#include <di/meta/list/quote.h>
#include <di/meta/list/transform.h>

namespace di::meta {
namespace detail {
    template<typename... Types>
    struct CartesianProductHelper {};

    template<>
    struct CartesianProductHelper<> : TypeConstant<List<List<>>> {};

    template<typename... Types>
    struct CartesianProductHelper<List<Types...>> : TypeConstant<List<List<Types>...>> {};

    template<typename... Ts, typename... Rest>
    struct CartesianProductHelper<List<Ts...>, Rest...>
        : TypeConstant<
              Concat<Transform<typename CartesianProductHelper<Rest...>::Type, BindBack<Quote<PushFront>, Ts>>...>> {};
}

template<concepts::TypeList... Types>
using CartesianProduct = detail::CartesianProductHelper<Types...>::Type;
}
