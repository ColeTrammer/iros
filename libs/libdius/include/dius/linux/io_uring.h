#pragma once

#include <dius/platform.h>

#ifdef DIUS_PLATFORM_LINUX

#include <di/prelude.h>
#include <dius/memory_region.h>
#include <dius/sync_file.h>

namespace dius::linux {
namespace io_uring {
    using SQE = struct io_uring_sqe;
    using CQE = struct io_uring_cqe;
    using SetupParams = struct io_uring_params;

    di::Result<int> sys_enter(unsigned int fd, unsigned int to_submit, unsigned int min_complete, unsigned int flags, void const* arg,
                              size_t arg_size);
    di::Result<int> sys_register(unsigned int fd, unsigned int op_code, void* arg, unsigned int nr_args);
    di::Result<SyncFile> sys_setup(u32 entries, SetupParams* params);

    class IoUringHandle {
    public:
        static di::Result<IoUringHandle> create();

        di::Optional<SQE&> get_next_sqe();
        di::Optional<CQE&> get_next_cqe();

        di::Result<void> submit_and_wait();

    private:
        IoUringHandle() = default;

        u32 sq_entry_count;
        u32 sq_mask;
        u32* sq_index_array;
        SQE* sq_array;

        di::Atomic<unsigned int> const* sq_head;
        di::Atomic<unsigned int>* sq_tail;
        di::Atomic<unsigned int> const* sq_flags;

        u32 cq_entry_count;
        u32 cq_mask;
        CQE* cq_array;

        di::Atomic<unsigned int>* cq_head;
        di::Atomic<unsigned int> const* cq_tail;
        di::Atomic<unsigned int> const* cq_overflow;

        SyncFile fd;
        MemoryRegion sq_region;
        MemoryRegion sqe_region;
        MemoryRegion cq_region;

        u32 sq_pending { 0 };
    };
}
}
#endif