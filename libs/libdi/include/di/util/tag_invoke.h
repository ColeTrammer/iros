#pragma once

#include <di/util/concepts/implicitly_convertible_to.h>
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

namespace concepts {
    template<typename Tag, typename... Args>
    concept TagInvocable = requires(Tag tag, Args&&... args) { di::util::tag_invoke(tag, util::forward<Args>(args)...); };
}

namespace meta {
    template<typename Tag, typename... Args>
    requires(concepts::TagInvocable<Tag, Args...>)
    using TagInvokeResult = decltype(di::util::tag_invoke(util::declval<Tag>(), util::declval<Args>()...));
}

namespace concepts {
    template<typename Tag, typename R, typename... Args>
    concept TagInvocableTo = TagInvocable<Tag, Args...> && ImplicitlyConvertibleTo<R, meta::TagInvokeResult<Tag, Args...>>;
}
}
