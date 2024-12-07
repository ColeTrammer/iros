#pragma once

#include "di/container/string/encoding.h"

namespace di::container::string {
template<concepts::Encoding Enc>
class StringViewImpl;
}
