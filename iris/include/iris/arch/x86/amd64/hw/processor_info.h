#pragma once

#include <di/prelude.h>

namespace iris {
enum class ProcessorFeatures {
    None,
    Smep = (1 << 0),
    Smap = (1 << 1),
};

DI_DEFINE_ENUM_BITWISE_OPERATIONS(ProcessorFeatures)

struct ProcessorInfo {
    ProcessorFeatures features { ProcessorFeatures::None };
    di::container::string::StringImpl<di::container::string::TransparentEncoding,
                                      di::StaticVector<char, decltype(12_zic)>>
        vendor_string;
};

ProcessorInfo detect_processor_info();
}
