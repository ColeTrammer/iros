#include <di/math/prelude.h>
#include <di/platform/compiler.h>
#include <di/util/prelude.h>
#include <iris/core/global_state.h>
#include <iris/core/print.h>
#include <iris/mm/address_space.h>
#include <iris/mm/page_frame_allocator.h>
#include <iris/mm/sections.h>

#if DI_GCC
#pragma GCC diagnostic ignored "-Wsized-deallocation"
#endif

// These functions are explicitly not to be used in the iris kernel.
// Nothrow new and sized deallocations are required throughout the kernel.
// void* operator new(std::size_t size);
// void* operator new(std::size_t size, std::align_val_t alignment);

void* operator new(std::size_t size, std::nothrow_t const&) noexcept {
    return ::operator new(size, std::align_val_t { 16 }, std::nothrow);
}

void* operator new(std::size_t size, std::align_val_t alignment, std::nothrow_t const&) noexcept {
    auto const& global_state = iris::global_state();
    return global_state.kernel_address_space.with_lock([&](auto& address_space) -> void* {
        ASSERT(!iris::interrupts_disabled() || !iris::current_processor_unsafe().is_online());

        auto& heap_region = global_state.inital_kernel_regions.back();
        auto old_heap_end = global_state.heap_end;

        auto alignment_difference = old_heap_end.raw_value() % di::to_underlying(alignment);
        if (alignment_difference != 0) {
            global_state.heap_end = old_heap_end + di::to_underlying(alignment) - alignment_difference;
        }

        auto result = global_state.heap_end;
        global_state.heap_end = result + size;

        if (old_heap_end == global_state.heap_start ||
            di::align_up(old_heap_end.raw_value(), 4096) != di::align_up(global_state.heap_end.raw_value(), 4096)) {
            auto virtual_start = iris::mm::VirtualAddress(di::align_up(old_heap_end.raw_value(), 4096));
            auto virtual_end = iris::mm::VirtualAddress(di::align_up(global_state.heap_end.raw_value(), 4096));
            for (auto virtual_address = virtual_start; virtual_address < virtual_end; virtual_address += 4096zu) {
                auto physical_page = iris::mm::allocate_page_frame();
                if (!physical_page) {
                    iris::println(u8"Failed to allocate physical page in ::new()"_sv);
                    return nullptr;
                }

                auto const page_number = (virtual_address - global_state.heap_start) / 4096;
                heap_region.backing_object().lock()->add_page(*physical_page, page_number);

                if (!address_space.map_physical_page(virtual_address, *physical_page,
                                                     iris::mm::RegionFlags::Readable |
                                                         iris::mm::RegionFlags::Writable)) {
                    iris::println(u8"Failed to map physical page in ::new()"_sv);
                    return nullptr;
                }
            }
            heap_region.set_end(virtual_end);
        }

        return result.void_pointer();
    });
}

// Deallocating delete.
void operator delete(void*, std::size_t) noexcept {
    ASSERT(!iris::interrupts_disabled() || !iris::current_processor()->is_online());
}
void operator delete(void*, std::align_val_t) noexcept {
    di::unreachable();
}
void operator delete(void*, std::size_t, std::align_val_t) noexcept {
    ASSERT(!iris::interrupts_disabled() || !iris::current_processor()->is_online());
}
