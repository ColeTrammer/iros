#pragma once

#include <di/container/path/prelude.h>
#include <iris/core/error.h>
#include <iris/fs/file.h>
#include <iris/uapi/initrd.h>
#include <iris/uapi/metadata.h>

namespace iris {
Expected<di::Tuple<di::Span<di::Byte const>, initrd::Type>> lookup_in_initrd(di::PathView path);

Expected<File> open_in_initrd(di::PathView path);
Expected<Metadata> path_metadata_in_initrd(di::PathView path);
}
