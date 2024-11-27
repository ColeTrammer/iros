#pragma once

#include <di/container/path/prelude.h>
#include <iris/core/error.h>
#include <iris/fs/file.h>
#include <iris/uapi/initrd.h>
#include <iris/uapi/metadata.h>

namespace iris {
auto init_initrd() -> Expected<void>;
}
