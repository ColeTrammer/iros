#pragma once

#include <di/container/interface/begin.h>
#include <di/container/interface/end.h>

namespace di::concepts {
template<typename T>
concept Container = requires(T& value) {
                        container::begin(value);
                        container::end(value);
                    };
}
