#pragma once

#include <di/function/prelude.h>
#include <di/util/prelude.h>
#include <di/vocab/span/prelude.h>
#include <iris/core/config.h>
#include <iris/core/error.h>
#include <iris/mm/virtual_address.h>

#include IRIS_ARCH_INCLUDE(core/userspace_access.h)

namespace iris {
Expected<void> copy_to_user(di::Span<byte const> kernel_data, byte* userspace_ptr);
Expected<void> copy_from_user(di::Span<byte const> userspace_data, byte* kernel_ptr);

Expected<void> validate_user_region(mm::VirtualAddress userspace_address, usize count, usize size);

template<di::concepts::Invocable F>
decltype(auto) with_userspace_access(F&& function) {
    UserspaceAccessEnabler guard {};
    return di::invoke(di::forward<F>(function));
}
}
