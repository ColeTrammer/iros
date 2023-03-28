#include <dius/thread.h>

namespace dius {
di::Result<Thread> Thread::do_start(di::Function<void()> entry) {
    auto platform = TRY(di::try_box<PlatformThread>());
    platform->entry = di::move(entry);

    auto result = pthread_create(
        &platform->native_handle, nullptr,
        [](void* entry) -> void* {
            auto* as_function = reinterpret_cast<di::Function<void()>*>(entry);
            (*as_function)();
            return nullptr;
        },
        reinterpret_cast<void*>(&platform->entry));
    if (result != 0) {
        return di::Unexpected(PosixError(-result));
    }
    return Thread(di::move(platform));
}

di::Result<void> PlatformThread::join() {
    auto result = pthread_join(native_handle, nullptr);
    native_handle = {};
    if (result != 0) {
        return di::Unexpected(PosixError(-result));
    }
    return {};
}
}
