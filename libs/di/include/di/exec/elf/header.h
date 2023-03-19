#pragma once

#include <di/exec/elf/types.h>

namespace di::exec {
template<ElfFormat format = ElfFormat::Native>
class ElfHeader {
private:
    using Types = ElfTypes<format>;
    using Byte = Types::Byte;
    using Half = Types::Half;
    using Word = Types::Word;
    using Addr = Types::Addr;
    using Off = Types::Off;

public:
    Byte ident[16];
    Half type;
    Half machine;
    Word version;
    Addr entry;
    Off program_table_off;
    Off section_table_off;
    Word flags;
    Half elf_header_size;
    Half program_entry_size;
    Half program_entry_count;
    Half section_entry_size;
    Half section_entry_count;
    Half string_table_section_index;
};
}
