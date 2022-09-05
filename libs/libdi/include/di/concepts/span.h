#pragma once

#include <di/container/vector/span_forward_declaration.h>
#include <di/meta/false_type.h>
#include <di/meta/remove_cvref.h>
#include <di/meta/true_type.h>
#include <di/types/size_t.h>

namespace di::concepts {
namespace detail {
    template<typename T>
    struct SpanHelper : meta::FalseType {};

    template<typename T, types::size_t extent>
    struct SpanHelper<di::container::Span<T, extent>> : meta::TrueType {};
}

template<typename T>
concept Span = detail::SpanHelper<meta::RemoveCV<T>>::value;
}
