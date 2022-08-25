#pragma once

namespace di::util::meta {
template<typename T>
using AddVolatile = T volatile;
}
