#include "dius/thread.h"

#include <linux/futex.h>
#include <linux/time.h>

#include "di/container/algorithm/prelude.h"
#include "di/math/prelude.h"
#include "di/sync/prelude.h"
#include "dius/system/process.h"
#include "dius/system/system_call.h"

namespace dius {
namespace this_thread {
    void sleep_for(di::Nanoseconds duration) {
        timespec timespec;
        timespec.tv_sec = di::duration_cast<di::Seconds>(duration).count();
        timespec.tv_nsec = duration.count() % long(di::Nanoseconds::Period::den);
        (void) system::system_call<i32>(system::Number::clock_nanosleep, CLOCK_MONOTONIC, 0, &timespec, nullptr);
    }

    void sleep_until(SteadyClock::TimePoint time_point) {
        timespec timespec;
        timespec.tv_sec = di::duration_cast<di::Seconds>(time_point.time_since_epoch()).count();
        timespec.tv_nsec = di::duration_cast<di::Nanoseconds>(time_point.time_since_epoch()).count() %
                           long(di::Nanoseconds::Period::den);
        (void) system::system_call<i32>(system::Number::clock_nanosleep, CLOCK_MONOTONIC, TIMER_ABSTIME, &timespec,
                                        nullptr);
    }
}

auto PlatformThread::create(runtime::TlsInfo info) -> di::Result<di::Box<PlatformThread, PlatformThreadDeleter>> {
    auto [tls_data, tls_size, tls_alignment] = info;

    auto alignment = di::max(tls_alignment, alignof(PlatformThread));
    auto size = di::align_up(tls_size, alignment) + sizeof(PlatformThread);
    auto* storage = reinterpret_cast<di::Byte*>(::operator new(size, std::align_val_t { alignment }, std::nothrow));
    ASSERT(storage);

    auto* thread_control_block = reinterpret_cast<PlatformThread*>(storage + di::align_up(tls_size, alignment));
    di::construct_at(thread_control_block);
    auto tls = thread_control_block->thread_local_storage(tls_size);

    di::copy(tls_data, tls.data());
    di::fill(*tls.last(tls_size - tls_data.size()), 0_b);

    return di::Box<PlatformThread, PlatformThreadDeleter>(thread_control_block);
}

void PlatformThreadDeleter::operator()(PlatformThread* thread) const {
    di::destroy_at(thread);

    auto [tls_data, tls_size, tls_alignment] = runtime::get_tls_info();

    auto alignment = di::max(tls_alignment, alignof(PlatformThread));
    auto size = di::align_up(tls_size, alignment) + sizeof(PlatformThread);

    auto* storage = reinterpret_cast<byte*>(thread) - di::align_up(tls_size, alignment);
    ::operator delete(storage, size, std::align_val_t(alignment));
}

auto Thread::do_start(di::Function<void()> entry) -> di::Result<Thread> {
    auto platform_thread = TRY(PlatformThread::create(runtime::get_tls_info()));
    platform_thread->entry = di::move(entry);

    constexpr auto stack_size = 0x20000_usize;

    auto* stack_start = TRY(system::system_call<byte*>(system::Number::mmap, nullptr, stack_size,
                                                       PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0));
    auto stack = di::Span { stack_start, stack_size };

    platform_thread->stack = MemoryRegion(stack);

    TRY(spawn_thread(*platform_thread));

    return Thread(di::move(platform_thread));
}

static auto futex_wait(int* futex, int expect) -> di::Result<void> {
    TRY(system::system_call<int>(system::Number::futex, futex, FUTEX_WAIT, expect, 0));
    return {};
}

auto PlatformThread::join() -> di::Result<void> {
    // Deadlock case: the current thread and this thread are the same.
    if (this == &PlatformThread::current()) {
        return di::Unexpected(di::BasicError::ResourceDeadlockWouldOccur);
    }

    while (auto value = di::AtomicRef(thread_id).load(di::MemoryOrder::Acquire)) {
        (void) futex_wait(&thread_id, value);
    }
    return {};
}
}
