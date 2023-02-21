#include <iris/core/print.h>
#include <iris/mm/page_frame_allocator.h>

namespace iris::mm {
// Store enough for 4 GiB of physical memory.
constexpr usize physical_page_count = 4llu * 1024u * 1024u * 1024u / 4096u;
static auto page_frame_bitmap = di::BitSet<physical_page_count> {};

void reserve_page_frames(PhysicalAddress base_address, usize page_count) {
    for (auto address = base_address; address < base_address + 4096 * page_count; address += 4096) {
        if (address.raw_address() / 4096 >= physical_page_count) {
            break;
        }
        page_frame_bitmap[address.raw_address() / 4096] = true;
    }
}

Expected<PhysicalAddress> allocate_page_frame() {
    for (usize i = 0; i < physical_page_count; i++) {
        if (!page_frame_bitmap[i]) {
            page_frame_bitmap[i] = true;
            return PhysicalAddress(i * 4096);
        }
    }
    return di::Unexpected(Error::NotEnoughMemory);
}

void deallocate_page_frame(PhysicalAddress address) {
    ASSERT(address.raw_address() % 4096 == 0);
    page_frame_bitmap[address.raw_address() / 4096] = false;
}
}
