#include <di/util/prelude.h>
#include <iris/mm/backing_object.h>
#include <iris/mm/physical_page.h>

namespace iris::mm {
void LockedBackingObject::add_page(PhysicalAddress address, u64 page_offset) {
    ASSERT(!m_pages.contains(page_offset));

    auto& page = physical_page(address);
    di::construct_at(&page.as_backed_page, page_offset);
    m_pages.insert(page.as_backed_page);
}
}
