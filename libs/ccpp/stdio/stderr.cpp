#include <ccpp/bits/file_implementation.h>

namespace ccpp {
constinit static auto stderr_storage = __file_implementation { dius::SyncFile(dius::SyncFile::Owned::No, 2),
                                                               nullptr,
                                                               0,
                                                               0,
                                                               0,
                                                               BufferMode::NotBuffered,
                                                               BufferOwnership::UserProvided,
                                                               ReadWriteMode::None,
                                                               Status::None,
                                                               Permissions::Readable | Permissions::Writable };
extern "C" {
constinit FILE* stderr = &stderr_storage;
}
}
