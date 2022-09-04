#pragma once

#include <di/container/interface/size.h>
#include <di/container/meta/container_iterator.h>
#include <di/container/meta/container_size_type.h>
#include <di/container/meta/iterator_ssize_type.h>
#include <di/meta/make_signed.h>
#include <di/util/tag_invoke.h>

namespace di::container {
struct SSizeFunction;

namespace detail {
    template<typename T>
    concept CustomSSize = concepts::TagInvocableTo<SSizeFunction, meta::IteratorSSizeType<meta::ContainerIterator<T>>, T>;

    template<typename T>
    concept SizeSSize = requires { typename meta::ContainerSizeType<T>; };
}

struct SSizeFunction {
    template<typename T>
    requires(detail::CustomSSize<T> || detail::SizeSSize<T>)
    constexpr meta::MakeUnsigned<meta::ContainerSizeType<T>> operator()(T&& container) const {
        if constexpr (detail::CustomSize<T>) {
            return util::tag_invoke(*this, util::forward<T>(container));
        } else {
            return static_cast<meta::MakeUnsigned<meta::ContainerSizeType<T>>>(size(util::forward<T>(container)));
        }
    }
};

constexpr inline auto ssize = SSizeFunction {};
}
