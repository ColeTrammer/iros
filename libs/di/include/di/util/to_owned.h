#pragma once

#include <di/function/pipeline.h>
#include <di/function/tag_invoke.h>
#include <di/meta/util.h>
#include <di/util/create.h>

namespace di::util {
namespace detail {
    template<typename T>
    concept MemberToOwned = requires(T&& value) { util::forward<T>(value).to_owned(); };

    struct ToOwnedFunction : function::pipeline::EnablePipeline {
        template<typename T>
        requires(concepts::TagInvocable<ToOwnedFunction, T> || MemberToOwned<T>)
        constexpr auto operator()(T&& value) const {
            if constexpr (concepts::TagInvocable<ToOwnedFunction, T>) {
                return function::tag_invoke(*this, util::forward<T>(value));
            } else {
                return util::forward<T>(value).to_owned();
            }
        }
    };
}

constexpr inline auto to_owned = detail::ToOwnedFunction {};

template<typename Self, typename T>
struct OwnedType {
public:
    constexpr auto to_owned() const { return util::to_owned(static_cast<Self const&>(*this)); }

private:
    template<concepts::RemoveCVRefSameAs<Self> This>
    constexpr friend auto tag_invoke(types::Tag<util::to_owned>, This&& self) {
        return util::create<T>(util::forward<This>(self));
    }
};
}

namespace di {
using util::to_owned;
}
