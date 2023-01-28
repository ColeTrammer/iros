#pragma once

#include <liim/forward.h>

namespace Unicode {
size_t terminal_code_point_width(uint32_t code_point);
size_t terminal_width(Utf8View text);
}
