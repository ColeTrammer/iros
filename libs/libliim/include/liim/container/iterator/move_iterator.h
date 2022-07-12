#pragma once

#include <liim/container/concepts.h>
#include <liim/container/iterator/wrapper_iterator.h>

namespace LIIM::Container::Iterators {
template<Iterator Iter>
class MoveIterator : public WrapperIteratorAdapter<MoveIterator<Iter>, Iter> {
public:
    using BaseValueType = IteratorTraits<Iter>::ValueType;
    static constexpr bool is_reference = IsReference<BaseValueType>::value;

    using ValueType = Conditional<is_reference, typename RemoveReference<BaseValueType>::type&&, BaseValueType>::type;

    constexpr explicit MoveIterator(Iter iter) : WrapperIteratorAdapter<MoveIterator, Iter>(move(iter)) {}

    constexpr ValueType operator*() { return move(*this->base()); }
};
}

using LIIM::Container::Iterators::MoveIterator;
