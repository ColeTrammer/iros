#pragma once

#include "iris/core/error.h"

namespace iris {
auto init_tmpfs() -> Expected<void>;
}
