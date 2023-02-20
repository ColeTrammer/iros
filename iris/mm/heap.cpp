#include <di/prelude.h>
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
    auto& global_state = iris::global_state();
    auto old_heap_end = global_state.heap_end;

    if (global_state.heap_end.raw_address() % di::to_underlying(alignment) != 0) {
        global_state.heap_end = global_state.heap_end + di::to_underlying(alignment);
        global_state.heap_end =
            iris::mm::VirtualAddress(global_state.heap_end.raw_address() & (~(di::to_underlying(alignment) - 1)));
    }

    auto result = global_state.heap_end;
    global_state.heap_end = global_state.heap_end + size;

    if (old_heap_end == global_state.heap_start ||
        (old_heap_end.raw_address() - 1) >> 12 != (global_state.heap_end.raw_address() - 1) >> 12) {
        auto virtual_start = iris::mm::VirtualAddress(old_heap_end.raw_address() / 4096 * 4096);
        auto virtual_end = iris::mm::VirtualAddress((global_state.heap_end.raw_address() + 4095) / 4096 * 4096);
        for (auto virtual_address = virtual_start; virtual_address < virtual_end; virtual_address += 4096) {

            auto& kernel_address_space = global_state.kernel_address_space;
            auto physical_page = iris::mm::allocate_page_frame().value();

            if (!kernel_address_space.map_physical_page(virtual_address, physical_page)) {
                iris::println(u8"Failed to map physical page in ::new()"_sv);
                return nullptr;
            }
        }
    }

    return result.void_pointer();
}

// Deallocating delete.
void operator delete(void*) noexcept {
    di::unreachable();
}
void operator delete(void*, std::size_t) noexcept {}
void operator delete(void*, std::align_val_t) noexcept {
    di::unreachable();
}
void operator delete(void*, std::size_t, std::align_val_t) noexcept {}