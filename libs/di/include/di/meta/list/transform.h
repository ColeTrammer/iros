#pragma once

#include <di/meta/list/concepts/meta_invocable.h>
#include <di/meta/list/concepts/valid_instantiation.h>
#include <di/meta/list/invoke.h>
#include <di/meta/list/list.h>

namespace di::meta {
namespace detail {
    template<typename...>
    struct TransformHelper {};

    template<typename... Types, typename Fun>
    requires(concepts::MetaInvocable<Fun> && (concepts::ValidInstantiation<Invoke, Fun, Types> && ...))
    struct TransformHelper<List<Types...>, Fun> : TypeConstant<List<Invoke<Fun, Types>...>> {};
}

template<concepts::TypeList List, typename Function>
using Transform = detail::TransformHelper<List, Function>::Type;
}
