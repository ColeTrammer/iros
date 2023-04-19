#pragma once

namespace dius::filesystem {
// NOTE: the FileType enum is setup to match the Iris MetadataType enum. Since only regular files and directories are
// supported, the rest of the enum is chosen arbitrarily.
enum class FileType {
    None = 128,
    NotFound = 255,
    Regular = 1,
    Directory = 2,
    Symlink = 3,
    Block = 4,
    Character = 5,
    Fifo = 6,
    Socket = 7,
    Unknown = 0,
};
}
