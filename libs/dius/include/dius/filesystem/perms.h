#pragma once

#include <di/util/bitwise_enum.h>

namespace dius::filesystem {
enum class Perms {
    None = 0,
    OwnerRead = 0400,
    OwnerWrite = 0200,
    OwnerExec = 0100,
    OwnerAll = 0700,
    GroupRead = 040,
    GroupWrite = 020,
    GroupExec = 010,
    GroupAll = 070,
    OthersRead = 04,
    OthersWrite = 02,
    OthersExec = 01,
    OthersAll = 07,
    All = 0777,
    SetUid = 04000,
    SetGid = 02000,
    StickyBit = 01000,
    Mask = 07777,
    Unknown = 0xFFFF,
};

DI_DEFINE_ENUM_BITWISE_OPERATIONS(Perms)
}
