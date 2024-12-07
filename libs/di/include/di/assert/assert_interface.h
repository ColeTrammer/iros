#pragma once

#ifndef DI_CUSTOM_ASSERT_HANDLER
#ifdef DI_NO_USE_STD
#error "Cannot use DI_NO_USE_STD with DI_CUSTOM_ASSERT_HANDLER"
#endif
#include <cstdlib>
#include <iostream>

#include "di/util/source_location.h"

namespace di::assert::detail {
[[noreturn]] inline void assert_fail(char const* source_text, char const* lhs_message, char const* rhs_message,
                                     util::SourceLocation loc) {
    std::cerr << "\033[31;1mASSERT\033[0m: " << source_text;
    std::cerr << ": " << loc.function_name() << ": " << loc.file_name() << ":" << loc.line() << ":" << loc.column()
              << std::endl;
    if (lhs_message) {
        std::cerr << "\033[1mLHS\033[0m: " << lhs_message << std::endl;
    }
    if (rhs_message) {
        std::cerr << "\033[1mRHS\033[0m: " << rhs_message << std::endl;
    }
    std::abort();
}
}
#else
#include "di/util/source_location.h"

namespace di::assert::detail {
void assert_fail(char const* source_text, char const* lhs_message, char const* rhs_message, util::SourceLocation loc);
}
#endif
