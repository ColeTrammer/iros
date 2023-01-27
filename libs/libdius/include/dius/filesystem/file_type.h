#pragma once

namespace dius::filesystem {
enum class FileType {
    None,
    NotFound,
    Regular,
    Directory,
    Symlink,
    Block,
    Character,
    Fifo,
    Socket,
    Unknown,
};
}