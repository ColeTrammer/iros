#pragma once

#include "di/any/container/any.h"

namespace di::any {
template<concepts::Interface Interface>
using AnyRef = Any<Interface, RefStorage>;
}

namespace di {
using any::AnyRef;
}
