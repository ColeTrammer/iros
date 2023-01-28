#pragma once

#include <di/meta/list/concepts/meta_invocable.h>
#include <di/meta/list/concepts/type_list.h>
#include <di/meta/list/invoke.h>
#include <di/meta/list/type.h>

namespace di::meta {
namespace detail {
    template<typename F, typename T>
    struct ApplyHelper;

    template<typename F, typename... Args>
    struct ApplyHelper<F, List<Args...>> : TypeConstant<Invoke<F, Args...>> {};
}

template<concepts::MetaInvocable F, concepts::TypeList T>
using Apply = Type<detail::ApplyHelper<F, T>>;
}