#include <dius/system/process.h>
#include <dius/system/system_call.h>
#include <dius/thread.h>
#include <linux/futex.h>

namespace dius {
di::Result<di::Box<PlatformThread, PlatformThreadDeleter>> PlatformThread::create(runtime::TlsInfo info) {
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

di::Result<Thread> Thread::do_start(di::Function<void()> entry) {
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

static di::Result<void> futex_wait(int* futex, int expect) {
    TRY(system::system_call<int>(system::Number::futex, futex, FUTEX_WAIT, expect, 0));
    return {};
}

di::Result<void> PlatformThread::join() {
    while (auto value = di::AtomicRef(thread_id).load(di::MemoryOrder::Acquire)) {
        (void) futex_wait(&thread_id, value);
    }
    return {};
}
}
