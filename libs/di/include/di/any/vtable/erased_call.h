#pragma once

#include <di/any/concepts/any_storable.h>
#include <di/any/concepts/method.h>
#include <di/any/concepts/method_callable_with.h>
#include <di/any/meta/method_signature.h>
#include <di/function/invoke.h>
#include <di/meta/algorithm.h>
#include <di/meta/language.h>
#include <di/meta/util.h>
#include <di/util/forward_like.h>

namespace di::any::detail {
template<typename Method, typename Storage, typename T>
struct ErasedCallImpl;

template<typename Tag, typename Storage, typename R, concepts::RemoveCVRefSameAs<This> Self, typename... BArgs,
         typename T>
struct ErasedCallImpl<Method<Tag, R(Self, BArgs...)>, Storage, T> {
    static R call(void* storage, BArgs... bargs) {
        using M = Method<Tag, R(Self, BArgs...)>;

        static_assert(concepts::AnyStorable<T, Storage>,
                      "Cannot create a vtable function for T not storable in Storage.");
        static_assert(concepts::MethodCallableWith<M, meta::Like<Self, T>>,
                      "Cannot create a vtable function because the Method is not callable for T.");

        auto const tag = Tag {};

        using QualifiedStorage = meta::MaybeConst<concepts::Const<meta::RemoveReference<This>>, Storage>;
        auto* typed_storage = reinterpret_cast<QualifiedStorage*>(storage);
        auto* object = typed_storage->template down_cast<meta::RemoveReference<T>>();

        if constexpr (concepts::TagInvocableTo<Tag const&, R, M, meta::Like<Self, T>, BArgs...>) {
            if constexpr (concepts::LanguageVoid<R>) {
                (void) tag_invoke(tag, M {}, util::forward_like<Self>(*object), util::forward<BArgs>(bargs)...);
            } else {
                return tag_invoke(tag, M {}, util::forward_like<Self>(*object), util::forward<BArgs>(bargs)...);
            }
        } else {
            return function::invoke_r<R>(tag, util::forward_like<Self>(*object), util::forward<BArgs>(bargs)...);
        }
    }
};
}
