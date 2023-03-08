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
void assert_write(char const* s, size_t n) {
    auto string_view = TransparentStringView(s, n);
    (void) dius::print("{}"_sv, string_view);
}

void assert_terminate() {
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
