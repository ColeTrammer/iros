#pragma once

#include <di/any/concepts/method.h>
#include <di/any/meta/method_tag.h>
#include <di/any/types/prelude.h>
#include <di/concepts/remove_cvref_same_as.h>
#include <di/function/invoke.h>
#include <di/meta/like.h>

namespace di::concepts {
namespace detail {
    template<typename M, typename T>
    struct MethodCallableWithHelper : meta::FalseType {};

    template<typename Tag, typename R, concepts::RemoveCVRefSameAs<This> Self, typename... BArgs, typename T>
    struct MethodCallableWithHelper<types::Method<Tag, R(Self, BArgs...)>, T> {
        constexpr static bool value = InvocableTo<Tag const&, R, meta::Like<Self, T>, BArgs...>;
    };
}

template<typename M, typename T>
concept MethodCallableWith = Method<M> && detail::MethodCallableWithHelper<M, T>::value;
}