#pragma once

#include "di/meta/operations.h"
#include "di/meta/util.h"
#include "di/util/declval.h"
#include "di/util/forward.h"

namespace di::function {
namespace tag_invoke_detail {
    void tag_invoke() = delete;

    struct TagInvokeFn {
        template<typename Tag, typename... Args>
        constexpr auto operator()(Tag tag, Args&&... args) const
            -> decltype(tag_invoke(static_cast<Tag&&>(tag), static_cast<Args&&>(args)...)) {
            return tag_invoke(static_cast<Tag&&>(tag), static_cast<Args&&>(args)...);
        }
    };
}

inline namespace tag_invoke_ns {
    constexpr inline tag_invoke_detail::TagInvokeFn tag_invoke {};
}
}

namespace di::types {
template<auto& T>
using Tag = di::meta::Decay<decltype(T)>;
}

namespace di::concepts {
template<typename Tag, typename... Args>
concept TagInvocable =
    requires(Tag tag, Args&&... args) { di::function::tag_invoke(tag, util::forward<Args>(args)...); };
}

namespace di::meta {
template<typename Tag, typename... Args>
requires(concepts::TagInvocable<Tag, Args...>)
using TagInvokeResult = decltype(di::function::tag_invoke(util::declval<Tag>(), util::declval<Args>()...));
}

namespace di::concepts {
template<typename Tag, typename R, typename... Args>
concept TagInvocableTo = TagInvocable<Tag, Args...> && ImplicitlyConvertibleTo<R, meta::TagInvokeResult<Tag, Args...>>;
}

namespace di {
using concepts::TagInvocable;
using concepts::TagInvocableTo;
using function::tag_invoke;
using meta::TagInvokeResult;
using types::Tag;
}
