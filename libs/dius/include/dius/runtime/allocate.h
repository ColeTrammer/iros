#pragma once

#include <di/assert/assert_bool.h>
#include <di/container/allocator/allocation_result.h>
#include <di/container/intrusive/forward_list.h>
#include <di/container/intrusive/forward_list_node.h>
#include <di/container/intrusive/list.h>
#include <di/sync/dumb_spinlock.h>
#include <di/sync/synchronized.h>
#include <di/util/destroy_at.h>
#include <di/vocab/array/array.h>
#include <di/vocab/error/result.h>
#include <di/vocab/tuple/tuple_forward_declaration.h>

namespace dius::runtime {
namespace detail {
    struct SizedTag : di::IntrusiveListTag<SizedTag> {
        template<typename U>
        constexpr static bool is_sized(di::InPlaceType<U>) {
            return true;
        }
    };

    struct FreeListNode : di::IntrusiveForwardListNode<> {};

    struct FreeList : di::IntrusiveListNode<SizedTag> {
        auto take_node() -> void* {
            ASSERT(!list.empty());

            auto* node = &*list.pop_front();
            di::destroy_at(node);
            return static_cast<void*>(node);
        }

        void add_node(void* pointer) {
            auto* node = static_cast<FreeListNode*>(pointer);
            di::construct_at(node);
            list.push_front(*node);
        }

        di::IntrusiveForwardList<FreeListNode> list;
    };

    class FreeListList {
    public:
        FreeListList() = default;

        auto allocate(usize size, usize align) -> di::Result<di::AllocationResult<>>;
        void deallocate(void* pointer);

    private:
        di::IntrusiveList<FreeList, SizedTag> m_list;
    };
}

class Heap {
public:
    static auto the() -> Heap&;

    auto allocate(usize size, usize align) -> di::Result<di::AllocationResult<>>;
    void deallocate(void* pointer, usize size, usize align);

private:
    Heap() = default;

    constexpr static auto block_sizes = di::Array {
        di::Tuple { 64zu, 64zu },     di::Tuple { 128zu, 128zu },    di::Tuple { 256zu, 256zu },
        di::Tuple { 512zu, 512zu },   di::Tuple { 1024zu, 1024zu },  di::Tuple { 2048zu, 2048zu },
        di::Tuple { 4096zu, 4096zu }, di::Tuple { 16384zu, 4096zu },
    };

    constexpr static auto block_size_count = block_sizes.size();

    constexpr static auto get_block_index(usize size, usize align) -> di::Optional<usize> {
        for (auto i = 0zu; i < block_size_count; ++i) {
            auto [block_size, block_align] = block_sizes[i];
            if (size <= block_size && align <= block_align) {
                return i;
            }
        }

        return {};
    }

    di::Synchronized<di::Array<detail::FreeListList, block_size_count>> m_blocks;
};
}
