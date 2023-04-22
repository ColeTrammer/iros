#include <iris/core/print.h>
#include <iris/mm/map_physical_address.h>
#include <iris/mm/page_frame_allocator.h>

#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic ignored "-Wstringop-overflow"
#endif

namespace iris::mm {
// Store enough for 4 GiB of physical memory.
constexpr usize physical_page_count = 4llu * 1024u * 1024u * 1024u / 4096u;

static auto page_frame_bitmap = di::Synchronized<di::BitSet<physical_page_count>> {};

void reserve_page_frames(PhysicalAddress base_address, usize page_count) {
    return page_frame_bitmap.with_lock([&](auto& bitmap) {
        for (auto address = base_address; address < base_address + 4096 * page_count; address += 4096) {
            if (address.raw_value() / 4096 >= physical_page_count) {
                break;
            }
            bitmap[address.raw_value() / 4096] = true;
        }
    });
}

Expected<PhysicalAddress> allocate_page_frame() {
    return page_frame_bitmap.with_lock([&](auto& bitmap) -> Expected<PhysicalAddress> {
        for (usize i = 0; i < physical_page_count; i++) {
            if (!bitmap[i]) {
                bitmap[i] = true;

                auto& array = TRY(map_physical_address(PhysicalAddress(i * 4096), 4096))
                                  .typed<di::Array<mm::PhysicalAddress, 512>>();
                array.fill(mm::PhysicalAddress(0));
                return PhysicalAddress(i * 4096);
            }
        }
        return di::Unexpected(Error::NotEnoughMemory);
    });
}

Expected<PhysicalAddress> allocate_physically_contiguous_page_frames(usize page_count) {
    return page_frame_bitmap.with_lock([&](auto& bitmap) -> Expected<PhysicalAddress> {
        for (usize i = 0; i <= physical_page_count - page_count; i++) {
            if (!bitmap[i]) {
                for (auto j : di::range(page_count)) {
                    if (bitmap[i + j]) {
                        // Skip to the next page, ensuring this algorithm is linear.
                        i += j + 1;
                        break;
                    }
                }
                for (auto k : di::range(page_count)) {
                    bitmap[i + k] = true;
                }
                return PhysicalAddress(i * 4096);
            }
        }
        return di::Unexpected(Error::NotEnoughMemory);
    });
}

void deallocate_page_frame(PhysicalAddress address) {
    ASSERT(address.raw_value() % 4096 == 0);
    return page_frame_bitmap.with_lock([&](auto& bitmap) {
        bitmap[address.raw_value() / 4096] = false;
    });
}
}
