#include <iris/core/global_state.h>
#include <iris/core/print.h>

namespace iris {
namespace cpuid {
    enum class Function : u32 { VendorId = 0, GetFeatureFlags = 7 };

    enum class FeatureFlagsEbx : u32 {
        Smap = (1 << 20),
        Smep = (1 << 7),
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

    iris::println("CPU Vendor ID: {}"_sv, processor_vendor_string);

    auto feature_flags_result = cpuid::query(cpuid::Function::GetFeatureFlags);

    bool supports_smep = !!(cpuid::FeatureFlagsEbx(feature_flags_result.ebx) & cpuid::FeatureFlagsEbx::Smep);
    bool supports_smap = !!(cpuid::FeatureFlagsEbx(feature_flags_result.ebx) & cpuid::FeatureFlagsEbx::Smap);

    iris::println("CPU feature flags: eax={} ebx={:#b} ecx={:#b} edx={:#b}"_sv, feature_flags_result.eax,
                  feature_flags_result.ebx, feature_flags_result.ecx, feature_flags_result.edx);

    auto features = ProcessorFeatures::None;
    if (supports_smep) {
        features |= ProcessorFeatures::Smep;
    }
    if (supports_smap) {
        features |= ProcessorFeatures::Smap;
    }
    return { features, processor_vendor_string };
}
}
