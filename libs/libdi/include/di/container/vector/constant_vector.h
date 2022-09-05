#pragma once

namespace di::concepts::detail {
template<typename T>
concept ConstantVector = requires(T& lvalue, T const& clvalue) {
                             lvalue.span();
                             clvalue.span();
                         };
}
