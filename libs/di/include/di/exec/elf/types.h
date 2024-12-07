#pragma once

#include "di/exec/elf/format.h"
#include "di/types/prelude.h"

namespace di::exec {
template<ElfFormat format = ElfFormat::Native>
struct ElfTypes {
    constexpr static auto endian = elf_format_endian(format);
    constexpr static auto is_64bit = elf_format_64bit(format);

    using Addr = StaticEndian<meta::Conditional<is_64bit, u64, u32>, endian>;
    using Off = StaticEndian<meta::Conditional<is_64bit, i64, i32>, endian>;
    using Half = StaticEndian<u16, endian>;
    using Word = StaticEndian<u32, endian>;
    using Sword = StaticEndian<i32, endian>;
    using Xword = StaticEndian<u64, endian>;
    using Sxword = StaticEndian<i64, endian>;
    using Byte = di::Byte;
};
}
