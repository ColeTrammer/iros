#pragma once

#include <di/prelude.h>

namespace iris::initrd {
using UUID = di::UUID;

constexpr inline auto signature = "b2bf882c-c789-4728-b4a0-b3b944078e29"_uuid;
constexpr inline auto block_size = usize(4096);
constexpr inline auto directory_entry_align = usize(8);

enum class Type : u8 {
    Regular = 0,
    Directory = 1,
};

struct DirectoryEntry {
    u32 block_offset;
    u32 byte_size;
    u16 next_entry;
    Type type;
    u8 name_length;
};

struct SuperBlock {
    UUID signature;
    UUID generation;
    u64 created_at_seconds_since_epoch;
    u32 version;
    u32 total_size;
    DirectoryEntry root_directory;
};
}