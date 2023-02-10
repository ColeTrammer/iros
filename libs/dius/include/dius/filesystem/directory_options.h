#pragma once

#include <di/util/bitwise_enum.h>

namespace dius::filesystem {
enum class DirectoryOptions {
    None = 0,
    FollowDirectorySymlink = 1,
    SkipPermissionDenied = 2,
};

DI_DEFINE_ENUM_BITWISE_OPERATIONS(DirectoryOptions)
}