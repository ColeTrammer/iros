#pragma once

#include <di/container/intrusive/prelude.h>
#include <di/vocab/pointer/prelude.h>

namespace iris::mm {
/// @brief A physical page of memory.
///
/// The kernel pre-allocates one of these for every physical page of memory in the system. This ensures that pages can
/// be tracked correctly, and prevents circular dependencies between the page frame allocator and the kernel heap. As a
/// consequence, this structure must be kept as small as possible.
class PhysicalPage
    : public di::IntrusiveRefCount<PhysicalPage>
    , public di::IntrusiveListNode<PhysicalPage> {};
}
