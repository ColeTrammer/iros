#pragma once

#include "di/types/integers.h"

namespace dius::tty {
struct WindowSize {
    u32 rows { 0 };
    u32 cols { 0 };
    u32 pixel_width { 0 };
    u32 pixel_height { 0 };
};
}
