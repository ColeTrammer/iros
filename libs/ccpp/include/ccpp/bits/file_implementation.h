#pragma once

#include <di/container/allocator/prelude.h>
#include <di/sync/prelude.h>
#include <di/vocab/expected/prelude.h>
#include <dius/error.h>
#include <dius/sync_file.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

namespace ccpp {
template<typename T>
struct MallocAllocator {
public:
    using Value = T;

    di::Expected<di::container::Allocation<T>, di::GenericCode> allocate(usize count) const {
        auto* data = [&] {
            if constexpr (alignof(T) > 16) {
                return aligned_alloc(alignof(T), count * sizeof(T));
            }
            return malloc(count * sizeof(T));
        }();

        if (!data) {
            return di::Unexpected(di::BasicError::NotEnoughMemory);
        }
        return di::container::Allocation<T> { static_cast<T*>(data), count };
    }

    void deallocate(T* pointer, usize) { free(pointer); }
};

struct FileDeleter {
    void operator()(FILE* file) const { (void) fclose(file); }
};

using FileHandle = di::Box<FILE, FileDeleter>;

enum class BufferMode {
    NotBuffered = _IONBF,
    LineBuffered = _IOLBF,
    FullBuffered = _IOFBF,
};

enum class Permissions {
    None = 0,
    Readable = 1,
    Writable = 2,
};

DI_DEFINE_ENUM_BITWISE_OPERATIONS(Permissions)

enum class BufferOwnership {
    Owned,
    UserProvided,
};

enum class ReadWriteMode {
    None = 0,
    Read = 1,
    Write = 2,
};

enum class Status {
    None = 0,
    Eof = 1,
    Error = 2,
};

DI_DEFINE_ENUM_BITWISE_OPERATIONS(Status)

struct File {
    explicit File() = default;

    constexpr explicit File(dius::SyncFile file_, byte* buffer_, usize buffer_capacity_, usize buffer_offset_,
                            usize buffer_size_, BufferMode buffer_mode_, BufferOwnership buffer_ownership_,
                            ReadWriteMode read_write_mode_, Status status_, Permissions permissions_)
        : file(di::move(file_))
        , buffer(buffer_)
        , buffer_capacity(buffer_capacity_)
        , buffer_offset(buffer_offset_)
        , buffer_size(buffer_size_)
        , buffer_mode(buffer_mode_)
        , buffer_ownership(buffer_ownership_)
        , read_write_mode(read_write_mode_)
        , status(status_)
        , permissions(permissions_) {}

    constexpr ~File() {
        if (buffer_ownership == BufferOwnership::Owned && buffer != nullptr) {
            MallocAllocator<byte>().deallocate(buffer, buffer_capacity);
        }
    }

    inline bool readable() const { return read_write_mode == ReadWriteMode::Read; }
    inline bool writable() const { return read_write_mode == ReadWriteMode::Write; }

    inline di::Expected<void, di::GenericCode> mark_as_readable() {
        if (!(permissions & Permissions::Readable)) {
            return di::Unexpected(di::BasicError::BadFileDescriptor);
        }
        if (read_write_mode == ReadWriteMode::Write) {
            return di::Unexpected(di::BasicError::BadFileDescriptor);
        }
        read_write_mode = ReadWriteMode::Read;
        return {};
    }

    inline di::Expected<void, di::GenericCode> mark_as_writable() {
        if (!(permissions & Permissions::Writable)) {
            return di::Unexpected(di::BasicError::BadFileDescriptor);
        }
        if (read_write_mode == ReadWriteMode::Read) {
            return di::Unexpected(di::BasicError::BadFileDescriptor);
        }
        read_write_mode = ReadWriteMode::Write;
        return {};
    }

    inline void mark_as_eof() {
        status = Status::Eof;
        read_write_mode = ReadWriteMode::None;
    }

    inline void mark_as_error() { status = Status::Error; }

    inline bool at_eof() const { return !!(status & Status::Eof); }
    inline bool has_error() const { return !!(status & Status::Error); }

    dius::SyncFile file;
    byte* buffer { nullptr };
    usize buffer_capacity { 0 };
    usize buffer_offset { 0 };
    usize buffer_size { 0 };
    BufferMode buffer_mode { BufferMode::FullBuffered };
    BufferOwnership buffer_ownership { BufferOwnership::Owned };
    ReadWriteMode read_write_mode { ReadWriteMode::None };
    Status status { Status::None };
    Permissions permissions { Permissions::None };
};

#define STDIO_TRY(...)                                           \
    __extension__({                                              \
        auto __result = (__VA_ARGS__);                           \
        if (!__result) {                                         \
            errno = di::to_underlying(__result.error().value()); \
            return EOF;                                          \
        }                                                        \
        di::util::move(__result).__try_did_succeed();            \
    }).__try_move_out()

#define STDIO_TRY_OR_MARK_ERROR(file, ...)                       \
    __extension__({                                              \
        auto __result = (__VA_ARGS__);                           \
        if (!__result) {                                         \
            errno = di::to_underlying(__result.error().value()); \
            (file).mark_as_error();                              \
            return EOF;                                          \
        }                                                        \
        di::util::move(__result).__try_did_succeed();            \
    }).__try_move_out()

#define STDIO_TRY_OR_NULL(...)                                   \
    __extension__({                                              \
        auto __result = (__VA_ARGS__);                           \
        if (!__result) {                                         \
            errno = di::to_underlying(__result.error().value()); \
            return nullptr;                                      \
        }                                                        \
        di::util::move(__result).__try_did_succeed();            \
    }).__try_move_out()

}

struct __file_implementation {
    /// @brief Get the file implementation.
    ///
    /// @warning This can only be called by the 'unlocked' stdio variants.
    ccpp::File& get_unlocked() { return locked.get_assuming_no_concurrent_accesses(); }

    template<typename... Args>
    // requires(di::concepts::ConstructibleFrom<ccpp::File, Args...>)
    constexpr explicit __file_implementation(Args&&... args) : locked(di::in_place, di::forward<Args>(args)...) {}

    di::Synchronized<ccpp::File> locked;
};
