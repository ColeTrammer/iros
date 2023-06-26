#pragma once

#include <di/cli/parser.h>
#include <di/container/string/string_view.h>

namespace pm {
struct Args {
    di::TransparentStringView package_name;

    constexpr static auto get_cli_parser() {
        return di::cli_parser<Args>("package_manager"_sv, "Iros package manager"_sv)
            .argument<&Args::package_name>("PACKAGE"_sv, "The package to manage"_sv, true);
    }
};
}
