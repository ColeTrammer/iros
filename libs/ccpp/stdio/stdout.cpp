#include <ccpp/bits/file_implementation.h>

namespace ccpp {
constinit static auto stdout_buffer = di::Array<byte, BUFSIZ> {};

constinit static auto stdout_storage = __file_implementation { dius::SyncFile(dius::SyncFile::Owned::No, 1),
                                                               stdout_buffer.data(),
                                                               stdout_buffer.size(),
                                                               0,
                                                               0,
                                                               BufferMode::LineBuffered,
                                                               BufferOwnership::UserProvided,
                                                               ReadWriteMode::None,
                                                               Status::None,
                                                               Permissions::Readable | Permissions::Writable };
extern "C" {
FILE* stdout = &stdout_storage;
}
}
