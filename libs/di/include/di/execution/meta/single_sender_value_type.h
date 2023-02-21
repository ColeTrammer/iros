#pragma once

#include <di/execution/meta/value_types_of.h>

namespace di::meta {
namespace detail {
    struct SingleSenderValueTypeHelperBadValue {};

    template<typename... Types>
    using SingleSenderValueTypeHelper = meta::Type<
        meta::Conditional<sizeof...(Types) == 0, meta::Id<void>,
                          meta::Conditional<sizeof...(Types) == 1, meta::Defer<meta::Front, meta::List<Types...>>,
                                            meta::Id<SingleSenderValueTypeHelperBadValue>>>>;
}

template<typename Send, typename Env = types::NoEnv>
requires(concepts::Sender<Send, Env> &&
         !concepts::SameAs<
             detail::SingleSenderValueTypeHelperBadValue,
             ValueTypesOf<Send, Env, detail::SingleSenderValueTypeHelper, detail::SingleSenderValueTypeHelper>>)
using SingleSenderValueType =
    ValueTypesOf<Send, Env, detail::SingleSenderValueTypeHelper, detail::SingleSenderValueTypeHelper>;
}
