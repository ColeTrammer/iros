#pragma once

namespace di::types {
namespace detail {
    template<typename T>
    struct DependentCompletionSignaturesImpl {
        struct Type {};
    };
}

template<typename T>
using DependentCompletionSignatures = detail::DependentCompletionSignaturesImpl<T>::Type;
}