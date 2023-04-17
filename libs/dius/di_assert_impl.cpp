#include <di/math/prelude.h>
#include <dius/print.h>
#include <dius/system/process.h>
#include <dius/test/prelude.h>

#ifndef DIUS_USE_RUNTIME
#include <cxxabi.h>
#include <execinfo.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#endif

namespace di::assert::detail {
static di::TransparentStringView zstring_to_string_view(char const* s) {
    return di::TransparentStringView(s, di::to_unsigned(di::distance(di::ZCString(s))));
}

void assert_fail(char const* source_text, char const* lhs_message, char const* rhs_message, util::SourceLocation loc) {
    auto source_text_view = zstring_to_string_view(source_text);

    dius::println("{}: {}"_sv, di::Styled("ASSERT"_sv, di::FormatColor::Red | di::FormatEffect::Bold),
                  source_text_view);

    dius::println("{}: {}(): {}:{}:{}"_sv, di::Styled("AT"_sv, di::FormatEffect::Bold),
                  zstring_to_string_view(loc.function_name()), zstring_to_string_view(loc.file_name()), loc.line(),
                  loc.column());
    if (lhs_message) {
        auto lhs_message_view = zstring_to_string_view(lhs_message);
        dius::println("{}: {}"_sv, di::Styled("LHS"_sv, di::FormatEffect::Bold), lhs_message_view);
    }
    if (rhs_message) {
        auto rhs_message_view = zstring_to_string_view(rhs_message);
        dius::println("{}: {}"_sv, di::Styled("RHS"_sv, di::FormatEffect::Bold), rhs_message_view);
    }

#ifndef DIUS_USE_RUNTIME
    void* storage[32];
    auto size = ::backtrace(storage, di::size(storage));

    auto** symbols = ::backtrace_symbols(storage, size);
    if (symbols) {
        for (int i = 0; i < size; i++) {
            auto* s = symbols[i];

            auto* start_of_symbol_name = ::strchr(s, '(') + 1;
            auto* end_of_symbol_name = ::strrchr(s, '+');
            auto end_of_symbol_save = di::exchange(*end_of_symbol_name, '\0');

            int status = -1;
            auto* demangled_name = abi::__cxa_demangle(start_of_symbol_name, nullptr, nullptr, &status);

            *end_of_symbol_name = end_of_symbol_save;
            if (status == 0) {
                *start_of_symbol_name = '\0';
                ::fprintf(stderr, "%s%s%s\n", s, demangled_name, end_of_symbol_name);
            } else {
                ::fprintf(stderr, "%s\n", symbols[i]);
            }
            free(demangled_name);
        }
    }

    ::free(symbols);
#endif

    auto& test_manager = dius::test::TestManager::the();
    if (test_manager.is_test_application()) {
        test_manager.handle_assertion_failure();
    }

    dius::system::exit_process(42);
}
}
