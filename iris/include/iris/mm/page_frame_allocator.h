#pragma once

#include <iris/core/error.h>
#include <iris/mm/physical_address.h>

namespace iris::mm {
void reserve_page_frames(PhysicalAddress base_address, size_t page_count);
Expected<PhysicalAddress> allocate_page_frame();
void deallocate_page_frame(PhysicalAddress);
}