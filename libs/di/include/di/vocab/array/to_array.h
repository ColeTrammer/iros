#pragma once

#include <di/meta/algorithm.h>
#include <di/meta/core.h>
#include <di/meta/language.h>
#include <di/meta/operations.h>
#include <di/util/move.h>
#include <di/vocab/array/array.h>

namespace di::vocab {
template<typename T, types::size_t size>
requires(concepts::CopyConstructible<T> && !concepts::LanguageArray<T>)
constexpr auto to_array(T (&array)[size]) {
    return [&]<types::size_t... indices>(meta::ListV<indices...>) {
        return Array<meta::RemoveCV<T>, size> { { array[indices]... } };
    }(meta::MakeIndexSequence<size> {});
}

template<typename T, types::size_t size>
requires(concepts::MoveConstructible<T> && !concepts::LanguageArray<T>)
constexpr Array<meta::RemoveCV<T>, size> to_array(T (&&array)[size]) {
    return [&]<types::size_t... indices>(meta::ListV<indices...>) {
        return Array<meta::RemoveCV<T>, size> { { util::move(array[indices])... } };
    }(meta::MakeIndexSequence<size> {});
}
}

namespace di {
using vocab::to_array;
}
