#pragma once

#include <di/container/string/prelude.h>
#include <di/types/prelude.h>
#include <iris/uapi/metadata.h>

namespace iris {
struct DirectoryRecord {
    u64 inode;
    u64 offset;
    u16 size;
    MetadataType type;
    u8 name_length;

    di::TransparentStringView name() const {
        return di::TransparentStringView { reinterpret_cast<char const*>(this + 1), name_length };
    }
};
}
