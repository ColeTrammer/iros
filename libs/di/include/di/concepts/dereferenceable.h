#pragma once

#include <di/concepts/can_reference.h>

namespace di::concepts {
template<typename T>
concept Dereferenceable = requires(T& it) {
                              { *it } -> CanReference;
                          };
}
