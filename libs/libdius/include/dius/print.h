#pragma once

#include <di/prelude.h>
#include <dius/sync_file.h>

namespace dius {
namespace detail {
    template<int fd>
    struct PrintFunction {
        template<typename... Args>
        void operator()(di::format::FormatStringImpl<di::container::string::Utf8Encoding, Args...> format_string, Args&&... args) const {
            auto fd_writer = SyncFile(SyncFile::Owned::No, fd);
            (void) di::vwriter_print<di::container::string::Utf8Encoding>(fd_writer, format_string, args...);
        }
    };

    template<int fd>
    struct PrintlnFunction {
        template<typename... Args>
        void operator()(di::format::FormatStringImpl<di::container::string::Utf8Encoding, Args...> format_string, Args&&... args) const {
            auto fd_writer = SyncFile(SyncFile::Owned::No, fd);
            (void) di::vwriter_println<di::container::string::Utf8Encoding>(fd_writer, format_string, args...);
        }
    };
}

constexpr inline auto print = detail::PrintFunction<1> {};
constexpr inline auto eprint = detail::PrintFunction<2> {};
constexpr inline auto dprint = detail::PrintFunction<2> {};

constexpr inline auto println = detail::PrintlnFunction<1> {};
constexpr inline auto eprintln = detail::PrintlnFunction<2> {};
constexpr inline auto dprintln = detail::PrintlnFunction<2> {};
}