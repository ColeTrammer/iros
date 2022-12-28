#pragma once

#include <dius/sync_file.h>

namespace dius {
namespace detail {
    template<int fd>
    struct LogFunction {
        template<typename... Args>
        void operator()(di::format::FormatStringImpl<di::container::string::Utf8Encoding, Args...> format_string, Args&&... args) const {
            auto fd_writer = SyncFile(SyncFile::Owned::No, fd);
            (void) di::vwriter_log<di::container::string::Utf8Encoding>(fd_writer, format_string, args...);
        }
    };
}

constexpr inline auto out_log = detail::LogFunction<1> {};
constexpr inline auto error_log = detail::LogFunction<2> {};
constexpr inline auto debug_log = detail::LogFunction<2> {};
}