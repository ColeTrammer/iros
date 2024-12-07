#pragma once

#include <iris/uapi/metadata.h>

#include "di/container/string/prelude.h"
#include "di/types/prelude.h"

namespace iris {
struct DirectoryRecord {
    u64 inode;
    u64 offset;
    u16 size;
    MetadataType type;
    u8 name_length;

    auto name() const -> di::TransparentStringView {
        return di::TransparentStringView { reinterpret_cast<char const*>(this + 1), name_length };
    }
};
}
