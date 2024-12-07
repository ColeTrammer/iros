#pragma once

#include "di/container/string/prelude.h"
#include "di/container/vector/prelude.h"
#include "di/meta/constexpr.h"
#include "di/types/prelude.h"
#include "di/util/bitwise_enum.h"

namespace iris {
enum class ProcessorFeatures {
    None = 0,
    Smep = (1 << 0),
    Smap = (1 << 1),
    Sse = (1 << 2),
    Sse2 = (1 << 3),
    Fxsr = (1 << 4),
    Mmx = (1 << 5),
    Sse3 = (1 << 6),
    Ssse3 = (1 << 7),
    Sse4_1 = (1 << 8),
    Sse4_2 = (1 << 9),
    Xsave = (1 << 10),
    Avx = (1 << 11),
    Avx2 = (1 << 12),
    Avx512 = (1 << 13),
    FsGsBase = (1 << 14),
    Apic = (1 << 15),
    X2Apic = (1 << 16),
    GibPages = (1 << 17),
};

DI_DEFINE_ENUM_BITWISE_OPERATIONS(ProcessorFeatures)

struct ProcessorInfo {
    ProcessorFeatures features { ProcessorFeatures::None };
    u32 fpu_max_state_size { 0 };
    u64 fpu_valid_xcr0 { 0 };
    di::container::string::StringImpl<di::container::string::TransparentEncoding,
                                      di::StaticVector<char, di::Constexpr<12ZU>>>
        vendor_string;

    void print_to_console();

    auto has_xsave() const -> bool {
        return (fpu_valid_xcr0 & 0b11) == 0b11 && !!(features & ProcessorFeatures::Xsave);
    }
    auto has_fs_gs_base() const -> bool { return !!(features & ProcessorFeatures::FsGsBase); }

    auto has_apic() const -> bool { return !!(features & ProcessorFeatures::Apic); }
    auto has_gib_pages() const -> bool { return !!(features & ProcessorFeatures::GibPages); }
};

auto detect_processor_info() -> ProcessorInfo;
}
