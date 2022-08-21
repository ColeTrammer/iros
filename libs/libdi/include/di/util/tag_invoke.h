#pragma once

#include <di/util/declval.h>
#include <di/util/forward.h>
#include <di/util/meta/decay.h>

namespace di::util {
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
    inline constexpr tag_invoke_detail::TagInvokeFn tag_invoke {};
}

template<auto& T>
using Tag = di::util::meta::Decay<decltype(T)>;

template<typename Tag, typename... Args>
concept TagInvokable = requires(Tag tag, Args&&... args) { di::util::tag_invoke(tag, util::forward<Args>(args)...); };

template<typename Tag, typename... Args>
requires(TagInvokable<Tag, Args...>)
using TagInvokeResult = decltype(di::util::tag_invoke(util::declval<Tag>(), util::declval<Args>()...));
}
