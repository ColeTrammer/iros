#include <dius/prelude.h>

namespace dius {
di::Result<di::Box<PlatformThread>> PlatformThread::create(runtime::TlsInfo) {
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

    // FIXME: this is incorrect, because the TCB needs a custom deleter.
    return di::Box(thread_control_block);
}
}
