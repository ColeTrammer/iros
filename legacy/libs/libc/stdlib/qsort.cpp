#include <liim/container/algorithm/sort.h>
#include <liim/container/iterator/continuous_iterator.h>
#include <liim/container/producer/range.h>

class CIterator : public ContinuousIteratorAdapter<CIterator> {
public:
    CIterator(void* base, size_t index, size_t size) : ContinuousIteratorAdapter(index), m_base(base), m_size(size) {}

    using ValueType = void*;

    ValueType operator*() { return static_cast<void*>(static_cast<char*>(m_base) + this->index() * m_size); }

    void swap_contents(CIterator& other) {
        uint8_t* a = static_cast<uint8_t*>(**this);
        uint8_t* b = static_cast<uint8_t*>(*other);
        for (auto i : range(m_size)) {
            a[i] ^= b[i];
            b[i] ^= a[i];
            a[i] ^= b[i];
        }
    }

private:
    void* m_base { nullptr };
    size_t m_size { 0 };
};

extern "C" void qsort(void* base, size_t nmemb, size_t size, int (*compar)(const void* a, const void* b)) {
    if (base == nullptr || nmemb == 0 || size == 0 || compar == nullptr) {
        return;
    }

    Alg::sort(iterator_container(CIterator(base, 0, size), CIterator(base, nmemb, size)), [&](auto* a, auto* b) {
        return compar(a, b) <=> 0;
    });
}
