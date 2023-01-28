#include <di/prelude.h>
#include <iris/core/print.h>
#include <iris/mm/address_space.h>
#include <iris/mm/page_frame_allocator.h>
#include <iris/mm/sections.h>

// These functions are explicitly not to be used in the iris kernel.
// Nothrow new and sized deallocations are required throughout the kernel.
// void* operator new(std::size_t size);
// void* operator new(std::size_t size, std::align_val_t alignment);

static auto const heap_start = iris::mm::VirtualAddress((iris::mm::kernel_end.raw_address() / 4096 * 4096) + 4096);
static auto heap_end = heap_start;

static inline u64 get_cr3() {
    u64 cr3;
    asm volatile("mov %%cr3, %%rdx\n"
                 "mov %%rdx, %0"
                 : "=m"(cr3)
                 :
                 : "rdx");
    return cr3;
}

void* operator new(std::size_t size, std::nothrow_t const&) noexcept {
    return ::operator new(size, std::align_val_t { 16 }, std::nothrow);
}

void* operator new(std::size_t size, std::align_val_t alignment, std::nothrow_t const&) noexcept {
    iris::println(u8"Trying to allocate size={} alignment={}"_sv, size, di::to_underlying(alignment));

    auto old_heap_end = heap_end;

    if (heap_end.raw_address() % di::to_underlying(alignment) != 0) {
        heap_end = heap_end + di::to_underlying(alignment);
        heap_end = iris::mm::VirtualAddress(heap_end.raw_address() & (~(di::to_underlying(alignment) - 1)));
    }

    iris::println(u8"Trying to allocate size={} alignment={}"_sv, size, di::to_underlying(alignment));

    auto result = heap_end;
    heap_end = heap_end + size;

    if (old_heap_end == heap_start || (old_heap_end.raw_address() - 1) >> 12 != (heap_end.raw_address() - 1) >> 12) {
        auto virtual_start = iris::mm::VirtualAddress(old_heap_end.raw_address() / 4096 * 4096);
        auto virtual_end = iris::mm::VirtualAddress((heap_end.raw_address() + 4095) / 4096 * 4096);
        for (auto virtual_address = virtual_start; virtual_address < virtual_end; virtual_address += 4096) {

            iris::mm::AddressSpace current(get_cr3() & ~0xFFFULL);
            auto physical_page = iris::mm::allocate_page_frame().value();
            iris::println(u8"Mapping physical page {} to {}"_sv, physical_page.raw_address(), virtual_address.raw_address());
            if (!current.map_physical_page(virtual_address, physical_page)) {
                iris::println(u8"Failed to map physical page in ::new()"_sv);
                return nullptr;
            }
            iris::println(u8"Done"_sv);
        }
    }

    return reinterpret_cast<void*>(result.raw_address());
}

// Deallocating delete.
void operator delete(void*) noexcept {
    di::unreachable();
}
void operator delete(void*, std::size_t) noexcept {}
void operator delete(void*, std::align_val_t) noexcept {}
void operator delete(void*, std::size_t, std::align_val_t) noexcept {}