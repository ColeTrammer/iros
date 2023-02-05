#pragma once

#include <di/any/types/prelude.h>
#include <di/function/tag_invoke.h>
#include <di/util/forward_like.h>

namespace di::any {
namespace detail {
    template<typename Tag, typename Sig>
    struct DispatcherImpl;

    template<typename Tag, typename R, concepts::DecaysTo<This> Self, typename... Args>
    struct DispatcherImpl<Tag, R(Self, Args...)> {
        using Type = Method<Tag, R(Self, Args...)>;

        template<typename T>
        requires(concepts::TagInvocableTo<Tag, R, T, Args...>)
        constexpr R operator()(T&& self, Args&&... args) const {
            auto const tag = Tag {};
            return function::tag_invoke(tag, util::forward_like<Self>(self), util::forward<Args>(args)...);
        }
    };
}

template<typename Self, concepts::LanguageFunction Sig>
using Dispatcher = detail::DispatcherImpl<Self, Sig>;
}