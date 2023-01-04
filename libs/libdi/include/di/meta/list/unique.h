#pragma once

#include <di/meta/list/fold.h>
#include <di/meta/list/list.h>
#include <di/meta/list/push_back.h>

namespace di::meta {
namespace detail {
    struct PushBackIfUnique {
        template<concepts::TypeList Lst, typename T>
        struct Impl : TypeConstant<PushBack<Lst, T>> {};

        template<concepts::TypeList Lst, typename T>
        requires(Contains<Lst, T>)
        struct Impl<Lst, T> : TypeConstant<Lst> {};

        template<concepts::TypeList Lst, typename T>
        using Invoke = Type<Impl<Lst, T>>;
    };
}

template<concepts::TypeList Lst>
using Unique = Fold<Lst, List<>, detail::PushBackIfUnique>;
}