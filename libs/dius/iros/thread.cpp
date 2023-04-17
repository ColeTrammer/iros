#include <dius/system/process.h>
#include <dius/system/system_call.h>
#include <dius/thread.h>

namespace dius {
di::Result<di::Box<PlatformThread, PlatformThreadDeleter>> PlatformThread::create(runtime::TlsInfo) {
    auto [tls_data, tls_size, tls_alignment] = runtime::get_tls_info();

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

    // FIXME: free this memory.
    auto* stack_start = TRY(system::system_call<byte*>(system::Number::allocate_memory, stack_size));
    auto stack = di::Span { stack_start, stack_size };

    platform_thread->stack = stack.data();

    auto id = TRY(system::system_call<int>(system::Number::create_task));
    platform_thread->thread_id = id;

    TRY(system::system_call<int>(system::Number::set_userspace_thread_pointer, id, platform_thread.get()));
    TRY(system::system_call<int>(system::Number::set_userspace_stack_pointer, id, stack.data() + stack.size() - 8));
    TRY(system::system_call<int>(
        system::Number::set_userspace_instruction_pointer, id, +[](void* closure) {
            auto* platform_thread = static_cast<PlatformThread*>(closure);
            platform_thread->entry();

            platform_thread->join_word.store(1, di::MemoryOrder::Release);
            system::exit_thread();
        }));
    TRY(system::system_call<int>(system::Number::set_userspace_argument1, id, platform_thread.get()));
    TRY(system::system_call<int>(system::Number::start_task, id));

    return Thread(di::move(platform_thread));
}

di::Result<void> PlatformThread::join() {
    while (!join_word.load(di::MemoryOrder::Acquire)) {
        ;
    }
    return {};
}
}
