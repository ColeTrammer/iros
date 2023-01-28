#pragma once

#include <liim/container/producer/range.h>
#include <liim/container/producer/sequence.h>
#include <liim/container/view/zip.h>

namespace LIIM::Container::View {
template<Container T>
constexpr auto enumerate(T&& container) {
    if constexpr (SizedContainer<T>) {
        return zip(range(container.size()), forward<T>(container));
    } else {
        return zip(sequence(static_cast<size_t>(0)), forward<T>(container));
    }
}
}

using LIIM::Container::View::enumerate;
