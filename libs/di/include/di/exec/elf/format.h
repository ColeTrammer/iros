#pragma once

#include "di/bit/endian/prelude.h"

namespace di::exec {
enum class ElfFormat {
    LittleEndian32 = 0,
    BigEndian32 = 1,
    LittleEndian64 = 2,
    BigEndian64 = 3,
    Native = sizeof(long) == 8
                 ? (Endian::Native == Endian::Little ? ElfFormat::LittleEndian64 : ElfFormat::BigEndian64)
                 : (Endian::Native == Endian::Little ? ElfFormat::LittleEndian32 : ElfFormat::BigEndian32),
};

constexpr static auto elf_format_endian(ElfFormat format) {
    return (format == ElfFormat::LittleEndian32 || format == ElfFormat::LittleEndian64) ? Endian::Little : Endian::Big;
}

constexpr static auto elf_format_64bit(ElfFormat format) {
    return format == ElfFormat::LittleEndian64 || format == ElfFormat::BigEndian64;
}
}
