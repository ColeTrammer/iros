#pragma once

#include <di/container/concepts/input_iterator.h>
#include <di/container/iterator/const_iterator_impl.h>
#include <di/meta/type_constant.h>

namespace di::meta {
namespace detail {
    template<typename Sent>
    struct ConstSentinelHelper : TypeConstant<Sent> {};

    template<concepts::InputIterator Iter>
    struct ConstSentinelHelper<Iter> : TypeConstant<container::ConstIteratorImpl<Iter>> {};
}

template<typename Sent>
using ConstSentinel = detail::ConstSentinelHelper<Sent>::Type;
}