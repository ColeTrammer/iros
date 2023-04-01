#include <ccpp/bits/file_implementation.h>

namespace ccpp {
constinit static auto stdin_buffer = di::Array<byte, BUFSIZ> {};

constinit static auto stdin_storage = __file_implementation { dius::SyncFile(dius::SyncFile::Owned::No, 0),
                                                              stdin_buffer.data(),
                                                              stdin_buffer.size(),
                                                              0,
                                                              0,
                                                              BufferMode::LineBuffered,
                                                              BufferOwnership::UserProvided,
                                                              ReadWriteMode::None,
                                                              Status::None,
                                                              Permissions::Readable | Permissions::Writable };
extern "C" {
FILE* stdin = &stdin_storage;
}
}
