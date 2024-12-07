#pragma once

#include <iris/uapi/initrd.h>
#include <iris/uapi/metadata.h>

#include "di/container/path/prelude.h"
#include "iris/core/error.h"
#include "iris/fs/file.h"

namespace iris {
auto init_initrd() -> Expected<void>;
}
