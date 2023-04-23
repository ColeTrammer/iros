#pragma once

#include <iris/core/error.h>
#include <iris/mm/physical_address.h>

namespace iris::mm {
void reserve_page_frames(PhysicalAddress base_address, usize page_count);
void unreserve_page_frames(PhysicalAddress base_address, usize page_count);
Expected<PhysicalAddress> allocate_page_frame();
Expected<PhysicalAddress> allocate_physically_contiguous_page_frames(usize page_count);
void deallocate_page_frame(PhysicalAddress);
}
