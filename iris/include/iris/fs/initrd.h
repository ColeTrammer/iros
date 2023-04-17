#pragma once

#include <di/container/path/prelude.h>
#include <iris/core/error.h>
#include <iris/fs/file.h>

namespace iris {
Expected<di::Span<di::Byte const>> lookup_in_initrd(di::PathView path);

Expected<File> open_in_initrd(di::PathView path);
}
