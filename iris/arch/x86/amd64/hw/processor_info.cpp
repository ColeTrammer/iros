#include <iris/core/global_state.h>
#include <iris/core/print.h>

namespace iris {
/// See this [link](https://sandpile.org/x86/cpuid.htm) for a list of CPU id queries and bits.
namespace cpuid {
    enum class Function : u32 {
        VendorId = 0x0,
        FamilyAndFlags = 0x1,
        GetFeatureFlags = 0x7,
        GetExtendedState = 0xD,
    };

    /// Corresponds to leaf ecx for [EAX=0000_00001h](https://sandpile.org/x86/cpuid.htm#level_0000_0001h).
    enum class FamilyFlagsEcx {
        Avx = (1 << 28),
        Xsave = (1 << 26),
        X2Apic = (1 << 21),
        Sse4_2 = (1 << 20),
        Sse4_1 = (1 << 19),
        Ssse3 = (1 << 9),
        Sse3 = (1 << 0),
    };

    DI_DEFINE_ENUM_BITWISE_OPERATIONS(FamilyFlagsEcx)

    /// Corresponds to leaf edx for [EAX=0000_00001h](https://sandpile.org/x86/cpuid.htm#level_0000_0001h).
    enum class FamilyFlagsEdx {
        Sse2 = (1 << 26),
        Sse = (1 << 25),
        Fxsr = (1 << 24),
        Mmx = (1 << 23),
        Apic = (1 << 9),
    };

    DI_DEFINE_ENUM_BITWISE_OPERATIONS(FamilyFlagsEdx)

    /// Corresponds to leaf ebx for [EAX=0000_00007h](https://sandpile.org/x86/cpuid.htm#level_0000_0007h).
    enum class FeatureFlagsEbx : u32 {
        Smap = (1 << 20),
        Avx512Foundations = (1 << 16),
        Smep = (1 << 7),
        Avx2 = (1 << 5),
        FsGsBase = (1 << 0),
    };

    DI_DEFINE_ENUM_BITWISE_OPERATIONS(FeatureFlagsEbx)

    struct Result {
        u32 eax;
        u32 ebx;
        u32 ecx;
        u32 edx;
    };

    inline Result query(Function function, u32 sublevel = 0) {
        u32 eax = di::to_underlying(function);
        u32 ebx = 0;
        u32 ecx = sublevel;
        u32 edx = 0;
        asm volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(eax), "b"(ebx), "c"(ecx), "d"(edx));
        return Result { eax, ebx, ecx, edx };
    }
}

ProcessorInfo detect_processor_info() {
    auto result = cpuid::query(cpuid::Function::VendorId);
    iris::println("CPU Maximum Supported CPUID Function: {}"_sv, result.eax);

    auto processor_vendor_string = di::container::string::StringImpl<di::container::string::TransparentEncoding,
                                                                     di::StaticVector<char, decltype(12_zic)>> {};
    (void) processor_vendor_string.push_back((result.ebx >> 0) & 0xFF);
    (void) processor_vendor_string.push_back((result.ebx >> 8) & 0xFF);
    (void) processor_vendor_string.push_back((result.ebx >> 16) & 0xFF);
    (void) processor_vendor_string.push_back((result.ebx >> 24) & 0xFF);
    (void) processor_vendor_string.push_back((result.edx >> 0) & 0xFF);
    (void) processor_vendor_string.push_back((result.edx >> 8) & 0xFF);
    (void) processor_vendor_string.push_back((result.edx >> 16) & 0xFF);
    (void) processor_vendor_string.push_back((result.edx >> 24) & 0xFF);
    (void) processor_vendor_string.push_back((result.ecx >> 0) & 0xFF);
    (void) processor_vendor_string.push_back((result.ecx >> 8) & 0xFF);
    (void) processor_vendor_string.push_back((result.ecx >> 16) & 0xFF);
    (void) processor_vendor_string.push_back((result.ecx >> 24) & 0xFF);

    auto family_and_flags_result = cpuid::query(cpuid::Function::FamilyAndFlags);

    auto supports_avx = !!(cpuid::FamilyFlagsEcx(family_and_flags_result.ecx) & cpuid::FamilyFlagsEcx::Avx);
    auto supports_xsave = !!(cpuid::FamilyFlagsEcx(family_and_flags_result.ecx) & cpuid::FamilyFlagsEcx::Xsave);
    auto supports_x2apic = !!(cpuid::FamilyFlagsEcx(family_and_flags_result.ecx) & cpuid::FamilyFlagsEcx::X2Apic);
    auto supports_sse4_2 = !!(cpuid::FamilyFlagsEcx(family_and_flags_result.ecx) & cpuid::FamilyFlagsEcx::Sse4_2);
    auto supports_sse4_1 = !!(cpuid::FamilyFlagsEcx(family_and_flags_result.ecx) & cpuid::FamilyFlagsEcx::Sse4_1);
    auto supports_ssse3 = !!(cpuid::FamilyFlagsEcx(family_and_flags_result.ecx) & cpuid::FamilyFlagsEcx::Ssse3);
    auto supports_sse3 = !!(cpuid::FamilyFlagsEcx(family_and_flags_result.ecx) & cpuid::FamilyFlagsEcx::Sse3);

    auto supports_sse2 = !!(cpuid::FamilyFlagsEdx(family_and_flags_result.edx) & cpuid::FamilyFlagsEdx::Sse2);
    auto supports_sse = !!(cpuid::FamilyFlagsEdx(family_and_flags_result.edx) & cpuid::FamilyFlagsEdx::Sse);
    auto supports_fxsr = !!(cpuid::FamilyFlagsEdx(family_and_flags_result.edx) & cpuid::FamilyFlagsEdx::Fxsr);
    auto supports_mmx = !!(cpuid::FamilyFlagsEdx(family_and_flags_result.edx) & cpuid::FamilyFlagsEdx::Mmx);
    auto supports_apic = !!(cpuid::FamilyFlagsEdx(family_and_flags_result.edx) & cpuid::FamilyFlagsEdx::Apic);

    auto feature_flags_result = cpuid::query(cpuid::Function::GetFeatureFlags);

    auto supports_fs_gs_base = !!(cpuid::FeatureFlagsEbx(feature_flags_result.ebx) & cpuid::FeatureFlagsEbx::FsGsBase);
    auto supports_smep = !!(cpuid::FeatureFlagsEbx(feature_flags_result.ebx) & cpuid::FeatureFlagsEbx::Smep);
    auto supports_smap = !!(cpuid::FeatureFlagsEbx(feature_flags_result.ebx) & cpuid::FeatureFlagsEbx::Smap);
    auto supports_avx2 = !!(cpuid::FeatureFlagsEbx(feature_flags_result.ebx) & cpuid::FeatureFlagsEbx::Avx2);
    auto supports_avx512 =
        !!(cpuid::FeatureFlagsEbx(feature_flags_result.ebx) & cpuid::FeatureFlagsEbx::Avx512Foundations);

    auto valid_xcr0 = 0_u64;
    auto fpu_max_size = 512_u32;

    // If the processor supports xsave, then the FPU size is dynamic. Otherwise, it is 512 bytes.
    if (supports_xsave) {
        auto extended_state_result = cpuid::query(cpuid::Function::GetExtendedState);

        valid_xcr0 = u64(extended_state_result.eax) | u64(extended_state_result.edx) << 32;
        fpu_max_size = extended_state_result.ecx;

        if (fpu_max_size <= 512) {
            println("WARNING: processor appears to have an invalid FPU state size of {}."
                    " Disabling extended FPU state support"_sv,
                    fpu_max_size);
            fpu_max_size = 512;
            valid_xcr0 = 0;
            supports_xsave = false;
        }
    }

    auto features = ProcessorFeatures::None;
    if (supports_smep) {
        features |= ProcessorFeatures::Smep;
    }
    if (supports_smap) {
        features |= ProcessorFeatures::Smap;
    }
    if (supports_avx2) {
        features |= ProcessorFeatures::Avx2;
    }
    if (supports_avx512) {
        features |= ProcessorFeatures::Avx512;
    }
    if (supports_fs_gs_base) {
        features |= ProcessorFeatures::FsGsBase;
    }
    if (supports_avx) {
        features |= ProcessorFeatures::Avx;
    }
    if (supports_xsave) {
        features |= ProcessorFeatures::Xsave;
    }
    if (supports_x2apic) {
        features |= ProcessorFeatures::X2Apic;
    }
    if (supports_sse4_2) {
        features |= ProcessorFeatures::Sse4_2;
    }
    if (supports_sse4_1) {
        features |= ProcessorFeatures::Sse4_1;
    }
    if (supports_ssse3) {
        features |= ProcessorFeatures::Ssse3;
    }
    if (supports_sse3) {
        features |= ProcessorFeatures::Sse3;
    }

    if (supports_sse2) {
        features |= ProcessorFeatures::Sse2;
    }
    if (supports_sse) {
        features |= ProcessorFeatures::Sse;
    }
    if (supports_fxsr) {
        features |= ProcessorFeatures::Fxsr;
    }
    if (supports_mmx) {
        features |= ProcessorFeatures::Mmx;
    }
    if (supports_apic) {
        features |= ProcessorFeatures::Apic;
    }

    return { features, fpu_max_size, valid_xcr0, processor_vendor_string };
}

void ProcessorInfo::print_to_console() {
    println("CPU Vendor String: {}"_sv, vendor_string);
    println("Max FPU State Size: {}"_sv, fpu_max_state_size);
    println("Valid xcr0: {:#b}"_sv, fpu_valid_xcr0);

    if (!!(features & ProcessorFeatures::Smep)) {
        println("Detected feature: {}"_sv, "smep"_sv);
    }
    if (!!(features & ProcessorFeatures::Smap)) {
        println("Detected feature: {}"_sv, "smap"_sv);
    }
    if (!!(features & ProcessorFeatures::Sse)) {
        println("Detected feature: {}"_sv, "sse"_sv);
    }
    if (!!(features & ProcessorFeatures::Sse2)) {
        println("Detected feature: {}"_sv, "sse2"_sv);
    }
    if (!!(features & ProcessorFeatures::Fxsr)) {
        println("Detected feature: {}"_sv, "fxsr"_sv);
    }
    if (!!(features & ProcessorFeatures::Mmx)) {
        println("Detected feature: {}"_sv, "mmx"_sv);
    }
    if (!!(features & ProcessorFeatures::Sse3)) {
        println("Detected feature: {}"_sv, "sse3"_sv);
    }
    if (!!(features & ProcessorFeatures::Sse4_1)) {
        println("Detected feature: {}"_sv, "sse4_1"_sv);
    }
    if (!!(features & ProcessorFeatures::Sse4_2)) {
        println("Detected feature: {}"_sv, "sse4_2"_sv);
    }
    if (!!(features & ProcessorFeatures::Xsave)) {
        println("Detected feature: {}"_sv, "xsave"_sv);
    }
    if (!!(features & ProcessorFeatures::Avx)) {
        println("Detected feature: {}"_sv, "avx"_sv);
    }
    if (!!(features & ProcessorFeatures::Avx2)) {
        println("Detected feature: {}"_sv, "avx2"_sv);
    }
    if (!!(features & ProcessorFeatures::Avx512)) {
        println("Detected feature: {}"_sv, "avx512"_sv);
    }
    if (!!(features & ProcessorFeatures::FsGsBase)) {
        println("Detected feature: {}"_sv, "fsgsbase"_sv);
    }
    if (!!(features & ProcessorFeatures::X2Apic)) {
        println("Detected feature: {}"_sv, "x2apic"_sv);
    }
    if (!!(features & ProcessorFeatures::Apic)) {
        println("Detected feature: {}"_sv, "apic"_sv);
    }
}
}
