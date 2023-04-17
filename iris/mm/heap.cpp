#include <di/math/prelude.h>
#include <iris/core/global_state.h>
#include <iris/core/print.h>
#include <iris/mm/address_space.h>
#include <iris/mm/page_frame_allocator.h>
#include <iris/mm/sections.h>

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

        auto old_heap_end = address_space.heap_end();

        auto alignment_difference = old_heap_end.raw_value() % di::to_underlying(alignment);
        if (alignment_difference != 0) {
            address_space.set_heap_end(old_heap_end + di::to_underlying(alignment) - alignment_difference);
        }

        auto result = address_space.heap_end();
        address_space.set_heap_end(result + size);

        if (old_heap_end == global_state.heap_start ||
            di::align_up(old_heap_end.raw_value(), 4096) != di::align_up(address_space.heap_end().raw_value(), 4096)) {
            auto virtual_start = iris::mm::VirtualAddress(di::align_up(old_heap_end.raw_value(), 4096));
            auto virtual_end = iris::mm::VirtualAddress(di::align_up(address_space.heap_end().raw_value(), 4096));
            for (auto virtual_address = virtual_start; virtual_address < virtual_end; virtual_address += 4096) {
                auto physical_page = iris::mm::allocate_page_frame().value();

                if (!address_space.map_physical_page(virtual_address, physical_page,
                                                     iris::mm::RegionFlags::Readable |
                                                         iris::mm::RegionFlags::Writable)) {
                    iris::println(u8"Failed to map physical page in ::new()"_sv);
                    return nullptr;
                }
            }
        }

        return result.void_pointer();
    });
}

// Deallocating delete.
void operator delete(void*) noexcept {
    di::unreachable();
}
void operator delete(void*, std::size_t) noexcept {
    ASSERT(!iris::interrupts_disabled() || !iris::current_processor()->is_online());
}
void operator delete(void*, std::align_val_t) noexcept {
    di::unreachable();
}
void operator delete(void*, std::size_t, std::align_val_t) noexcept {
    ASSERT(!iris::interrupts_disabled() || !iris::current_processor()->is_online());
}
