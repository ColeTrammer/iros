#pragma once

#include <di/exec/elf/types.h>
#include <di/util/bitwise_enum.h>

namespace di::exec {
enum class ElfProgramHeaderType : i32 {
    Null = 0,
    Load = 1,
    Dynamic = 2,
    Interpreter = 3,
    Note = 4,
    Shlib = 5,
    ProgramHeader = 6,
    Tls = 7,
    GnuEhFrame = 0x6474e550,
    GnuStack = 0x6474e551,
    GnuRelRo = 0x6474e552,
    GnuProperty = 0x6474e552,
};

enum class ElfProgramHeaderFlags : i32 {
    Executable = 1,
    Writable = 2,
    Readable = 4,
};

DI_DEFINE_ENUM_BITWISE_OPERATIONS(ElfProgramHeaderFlags)

template<ElfFormat format = ElfFormat::Native>
class ElfProgramHeader;

template<ElfFormat format>
requires(!elf_format_64bit(format))
class ElfProgramHeader<format> {
private:
    constexpr static auto endian = elf_format_endian(format);

    using Types = ElfTypes<format>;
    using Byte = Types::Byte;
    using Half = Types::Half;
    using Word = Types::Word;
    using Addr = Types::Addr;
    using Off = Types::Off;

public:
    StaticEndian<ElfProgramHeaderType, endian> type;
    Off offset;
    Addr virtual_addr;
    Addr physical_addr;
    Word file_size;
    Word memory_size;
    StaticEndian<ElfProgramHeaderFlags, endian> flags;
    Word align;
};

template<ElfFormat format>
requires(elf_format_64bit(format))
class ElfProgramHeader<format> {
private:
    constexpr static auto endian = elf_format_endian(format);

    using Types = ElfTypes<format>;
    using Byte = Types::Byte;
    using Half = Types::Half;
    using Word = Types::Word;
    using Addr = Types::Addr;
    using Off = Types::Off;
    using Xword = Types::Xword;

public:
    StaticEndian<ElfProgramHeaderType, endian> type;
    StaticEndian<ElfProgramHeaderFlags, endian> flags;
    Off offset;
    Addr virtual_addr;
    Addr physical_addr;
    Xword file_size;
    Xword memory_size;
    Xword align;
};
}
