#pragma once

#include <di/concepts/copy_constructible.h>
#include <di/concepts/language_array.h>
#include <di/concepts/move_constructible.h>
#include <di/meta/index_sequence.h>
#include <di/meta/make_index_sequence.h>
#include <di/meta/remove_cv.h>
#include <di/util/move.h>
#include <di/vocab/array/array.h>

namespace di::vocab {
template<typename T, types::size_t size>
requires(concepts::CopyConstructible<T> && !concepts::LanguageArray<T>)
constexpr auto to_array(T (&array)[size]) {
    return [&]<types::size_t... indices>(meta::IndexSequence<indices...>) {
        return Array<meta::RemoveCV<T>, size> { { array[indices]... } };
    }
    (meta::MakeIndexSequence<size> {});
}

template<typename T, types::size_t size>
requires(concepts::MoveConstructible<T> && !concepts::LanguageArray<T>)
constexpr Array<meta::RemoveCV<T>, size> to_array(T (&&array)[size]) {
    return [&]<types::size_t... indices>(meta::IndexSequence<indices...>) {
        return Array<meta::RemoveCV<T>, size> { { util::move(array[indices])... } };
    }
    (meta::MakeIndexSequence<size> {});
}
}
