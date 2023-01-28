#pragma once

#include <di/function/invoke.h>
#include <di/meta/index_sequence.h>
#include <di/meta/make_index_sequence.h>
#include <di/types/size_t.h>
#include <di/util/forward.h>
#include <di/util/get.h>
#include <di/vocab/tuple/tuple_like.h>
#include <di/vocab/tuple/tuple_size.h>

namespace di::vocab {
namespace detail {
    template<types::size_t... indices, typename F, concepts::TupleLike Tup>
    constexpr auto apply_impl(meta::IndexSequence<indices...>, F&& f, Tup&& tuple)
        -> decltype(function::invoke(util::forward<F>(f), util::get<indices>(util::forward<Tup>(tuple))...)) {
        return function::invoke(util::forward<F>(f), util::get<indices>(util::forward<Tup>(tuple))...);
    }
}

template<typename F, concepts::TupleLike Tup>
constexpr auto apply(F&& f, Tup&& tuple)
    -> decltype(detail::apply_impl(meta::MakeIndexSequence<meta::TupleSize<Tup>> {}, util::forward<F>(f),
                                   util::forward<Tup>(tuple))) {
    return detail::apply_impl(meta::MakeIndexSequence<meta::TupleSize<Tup>> {}, util::forward<F>(f),
                              util::forward<Tup>(tuple));
}
}
