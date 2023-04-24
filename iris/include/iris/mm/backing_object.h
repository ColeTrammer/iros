#pragma once

#include <di/container/intrusive/prelude.h>
#include <di/sync/prelude.h>
#include <iris/core/interruptible_spinlock.h>
#include <iris/mm/physical_address.h>
#include <iris/mm/physical_page.h>

namespace iris::mm {
class LockedBackingObject {
public:
    void add_page(mm::PhysicalAddress address, u64 page_offset);

private:
    di::IntrusiveTreeSet<BackedPhysicalPage, BackedPhysicalPageTreeTag> m_pages;
};

class BackingObject
    : public di::IntrusiveRefCount<BackingObject>
    , public di::Synchronized<LockedBackingObject, InterruptibleSpinlock> {};
}
