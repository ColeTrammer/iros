#pragma once

namespace di::concepts::detail {
template<typename T>
concept ConstantVector = requires(T& lvalue, T const& clvalue) {
    typename T::Value;
    typename T::ConstValue;
    lvalue.span();
    clvalue.span();
};
}

namespace di::meta::detail {
template<concepts::detail::ConstantVector T>
using VectorValue = T::Value;

template<concepts::detail::ConstantVector T>
using VectorConstValue = T::ConstValue;

template<concepts::detail::ConstantVector T>
using VectorIterator = T::Value*;

template<concepts::detail::ConstantVector T>
using VectorConstIterator = T::ConstValue*;
}
