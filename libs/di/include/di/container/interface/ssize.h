#pragma once

#include "di/container/interface/size.h"
#include "di/container/meta/container_iterator.h"
#include "di/container/meta/container_size_type.h"
#include "di/container/meta/iterator_ssize_type.h"
#include "di/function/tag_invoke.h"
#include "di/meta/language.h"

namespace di::container {
struct SSizeFunction;

namespace detail {
    template<typename T>
    concept CustomSSize =
        concepts::TagInvocableTo<SSizeFunction, meta::IteratorSSizeType<meta::ContainerIterator<T>>, T>;

    template<typename T>
    concept SizeSSize = requires { typename meta::ContainerSizeType<T>; };
}

struct SSizeFunction : function::pipeline::EnablePipeline {
    template<typename T>
    requires(detail::CustomSSize<T> || detail::SizeSSize<T>)
    constexpr auto operator()(T&& container) const -> meta::MakeSigned<meta::ContainerSizeType<T>> {
        if constexpr (detail::CustomSize<T>) {
            return function::tag_invoke(*this, util::forward<T>(container));
        } else {
            return static_cast<meta::MakeSigned<meta::ContainerSizeType<T>>>(size(util::forward<T>(container)));
        }
    }
};

constexpr inline auto ssize = SSizeFunction {};
}

namespace di {
using container::ssize;
}
