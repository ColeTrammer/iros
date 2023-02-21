#pragma once

namespace dius::filesystem {
enum class FileType {
    // NOTE: the FileType enum is setup to match the Linux system call ABI.
    //       None and NotFound are chosen by us, and are never returned by Linux.
    None = 64,
    NotFound = 32,
    Regular = 8,
    Directory = 4,
    Symlink = 10,
    Block = 6,
    Character = 2,
    Fifo = 1,
    Socket = 12,
    Unknown = 0,
};
}
