#pragma once

#include <di/container/hash/node/hash_node.h>
#include <di/container/intrusive/forward_list_node.h>
#include <di/container/iterator/iterator_base.h>
#include <di/container/iterator/next.h>
#include <di/container/meta/container_iterator.h>
#include <di/container/types/prelude.h>
#include <di/types/prelude.h>
#include <di/vocab/span/prelude.h>

namespace di::container {
template<typename Value, typename Tag>
class HashNodeIterator : public IteratorBase<HashNodeIterator<Value, Tag>, ForwardIteratorTag, Value, isize> {
private:
    using Node = HashNode<Tag>;
    using ConcreteNode = decltype(Tag::node_type(in_place_type<Value>));
    using Bucket = container::IntrusiveForwardList<Node, Tag>;
    using BucketIter = meta::ContainerIterator<Bucket>;

public:
    HashNodeIterator() = default;

    constexpr explicit HashNodeIterator(vocab::Span<Bucket const> buckets, usize bucket_index)
        : m_buckets({ const_cast<Bucket*>(buckets.data()), buckets.size() }), m_bucket_index(bucket_index) {
        if (m_bucket_index < m_buckets.size()) {
            m_before_current = m_buckets[m_bucket_index].before_begin();
        }
    }

    constexpr explicit HashNodeIterator(vocab::Span<Bucket const> buckets, usize bucket_index,
                                        BucketIter current_current)
        : m_buckets({ const_cast<Bucket*>(buckets.data()), buckets.size() })
        , m_bucket_index(bucket_index)
        , m_before_current(current_current) {}

    constexpr Value& operator*() const {
        DI_ASSERT(m_before_current != BucketIter {});
        return Tag::down_cast(in_place_type<Value>,
                              static_cast<ConcreteNode&>(*container::next(m_before_current).node()));
    }
    constexpr Value* operator->() const { return util::addressof(**this); }

    constexpr void advance_one() {
        DI_ASSERT(m_before_current != BucketIter {});
        ++m_before_current;
        if (container::next(m_before_current) != m_buckets[m_bucket_index].end()) {
            return;
        }

        ++m_bucket_index;
        while (m_bucket_index < m_buckets.size()) {
            m_before_current = m_buckets[m_bucket_index].before_begin();
            if (container::next(m_before_current) != m_buckets[m_bucket_index].end()) {
                return;
            }

            ++m_bucket_index;
        }

        m_before_current = {};
    }

    constexpr BucketIter before_current() const { return m_before_current; }
    constexpr usize bucket_index() const { return m_bucket_index; }
    constexpr Node& node() const { return static_cast<Node&>(*container::next(m_before_current).node()); }

private:
    constexpr friend bool operator==(HashNodeIterator const& a, HashNodeIterator const& b) {
        return a.m_before_current == b.m_before_current;
    }

    vocab::Span<Bucket> m_buckets {};
    usize m_bucket_index { 0 };
    BucketIter m_before_current {};
};
}
