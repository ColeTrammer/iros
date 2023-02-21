#pragma once

#include <di/prelude.h>

namespace iris::x86::amd64::page_structure {
using namespace di::bit;

// The format of the amd64 paging structures in long mode is described
// in the AMD64 Programmer's Manual; Volume 2; Figure 5-18, 5-19, 5-20, and 5-21.
struct Present : BitFlag<0> {};
struct Writable : BitFlag<1> {};
struct User : BitFlag<2> {};

// This bit is obsolete in the amd64 architecture. See Table 7-4 from the amd manual.
struct WriteThrough : BitFlag<3> {};

// This bit allows disabling the cache when reading and writing to memory. This
// may be necessary on real-hardware when dealing with memory-mapped IO registers.
struct CacheDisable : BitFlag<4> {};

// This bit is set by the CPU when a corresponding section of memory is accessed.
// This can be used to collect memory usage decisions to effect swap and disk cache
// behavior.
struct Accessed : BitFlag<5> {};

// This bit is set by the CPU when a write has been made to an individual page.
// This can be used to know whether pages need to be flushed to disk when using
// memory-mapped file IO.
struct Dirty : BitFlag<6> {};

// This bit enables either 2 MiB paging or 1 GiB paging, depending on which tier
// entry is being used.
struct HugePage : BitFlag<7> {};

// This bit prevents TLB flushed (reloading cr3) from effecting these pages.
// This is ideally used to kernel pages, which are present in all address spaces.
// However, some mitigations for spectre and meltdown may restrict its usage.
struct Global : BitFlag<8> {};

// This is the structure which stores the actual physical address which the
// CPU accesses. It must be page-aligned, and the lowest 12 bits must be dropped.
struct PhysicalAddress : BitField<12, 40> {};

struct NotExecutable : BitFlag<63> {};

using StructureEntry =
    BitStruct<8, Present, Writable, User, WriteThrough, CacheDisable, Accessed, PhysicalAddress, NotExecutable>;

using PageStructureTable = di::Array<StructureEntry, 512>;

// This includes the extra fields from the regular page structure: dirty, huge, global
using FinalEntry = BitStruct<8, Present, Writable, User, WriteThrough, CacheDisable, Accessed, Dirty, HugePage, Global,
                             PhysicalAddress, NotExecutable>;

using FinalTable = di::Array<StructureEntry, 512>;

// The translation of virtual addresses to physical addresses is defined illustrated
// in Figure 5-17 of the AMD64 Programmer's Manual; Volume 2.
struct Pml4Offset : BitField<39, 9> {};
struct PdpOffset : BitField<30, 9> {};
struct PdOffset : BitField<21, 9> {};
struct PtOffset : BitField<12, 9> {};
struct PhysicalOffset : BitField<0, 12> {};

using VirtualAddressStructure = BitStruct<8, Pml4Offset, PdpOffset, PdOffset, PtOffset, PhysicalOffset>;
}
