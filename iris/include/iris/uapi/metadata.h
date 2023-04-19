#pragma once

#include <di/types/prelude.h>

namespace iris {
enum class MetadataType : u8 {
    Unknown = 0,
    Regular = 1,
    Directory = 2,
};

struct Metadata {
    MetadataType type;
    u64 size;
};
}
