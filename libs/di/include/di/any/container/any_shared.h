#pragma once

#include <di/any/container/any.h>

namespace di::any {
template<concepts::Interface Interface>
using AnyShared = Any<Interface, SharedStorage>;
}
