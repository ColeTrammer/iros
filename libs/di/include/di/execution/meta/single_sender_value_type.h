#pragma once

#include "di/execution/concepts/sender_in.h"
#include "di/execution/meta/value_types_of.h"

namespace di::meta {
namespace detail {
    struct SingleSenderValueTypeHelperBadValue {};

    template<typename... Types>
    using SingleSenderValueTypeHelper = meta::Type<
        meta::Conditional<sizeof...(Types) == 0, meta::TypeConstant<void>,
                          meta::Conditional<sizeof...(Types) == 1, meta::Defer<meta::Front, meta::List<Types...>>,
                                            meta::TypeConstant<SingleSenderValueTypeHelperBadValue>>>>;
}

template<typename Send, typename Env>
requires(concepts::SenderIn<Send, Env> &&
         !concepts::SameAs<
             detail::SingleSenderValueTypeHelperBadValue,
             ValueTypesOf<Send, Env, detail::SingleSenderValueTypeHelper, detail::SingleSenderValueTypeHelper>>)
using SingleSenderValueType =
    ValueTypesOf<Send, Env, detail::SingleSenderValueTypeHelper, detail::SingleSenderValueTypeHelper>;
}
