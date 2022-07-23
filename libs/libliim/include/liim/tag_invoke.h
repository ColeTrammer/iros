#pragma once

#include <liim/utilities.h>

namespace LIIM {
namespace TagInvokeDetail {
    void tag_invoke() = delete;

    struct TagInvokeFn {
        template<typename Tag, typename... Args>
        constexpr auto operator()(Tag tag, Args&&... args) const
            -> decltype(tag_invoke(static_cast<Tag&&>(tag), static_cast<Args&&>(args)...)) {
            return tag_invoke(static_cast<Tag&&>(tag), static_cast<Args&&>(args)...);
        }
    };
}

inline namespace TagInvoke {
    inline constexpr TagInvokeDetail::TagInvokeFn tag_invoke {};
}

template<auto& T>
using Tag = decay_t<decltype(T)>;

template<typename Tag, typename... Args>
concept TagInvokable = requires {
    LIIM::tag_invoke(declval<Tag>(), declval<Args>()...);
};

template<typename Tag, typename... Args>
requires(TagInvokable<Tag, Args...>) using TagInvokeResult = decltype(LIIM::tag_invoke(declval<Tag>(), declval<Args>()...));
}

using LIIM::Tag;
using LIIM::TagInvokable;
using LIIM::TagInvokeResult;
