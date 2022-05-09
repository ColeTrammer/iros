#pragma once

#include <liim/forward.h>
#include <liim/linked_list.h>
#include <liim/maybe.h>
#include <liim/traits.h>
#include <liim/vector.h>

namespace LIIM {
template<Hashable T>
class HashSet {
public:
    explicit HashSet(int num_buckets = 20) : m_buckets(Vector<LinkedList<T>>(num_buckets)) {}

    HashSet(const HashSet<T>& other) = default;

    HashSet(HashSet<T>&& other) : m_buckets(move(other.m_buckets)), m_size(exchange(other.m_size, 0)) {}

    void clear() {
        m_buckets.for_each([&](auto& l) {
            l.clear();
        });
    }

    HashSet<T>& operator=(const HashSet<T>& other) {
        if (this != &other) {
            HashSet<T> temp(other);
            swap(temp);
        }

        return *this;
    }

    HashSet<T>& operator=(HashSet<T>&& other) {
        if (this != &other) {
            HashSet<T> temp(move(other));
            swap(temp);
        }

        return *this;
    }

    void put(const T& val) {
        if (m_buckets.size() == 0) {
            for (int i = 0; i < m_buckets.capacity(); i++) {
                m_buckets.add(LinkedList<T>());
            }
        }

        auto slot = get(val);
        if (slot) {
            slot->~T();
            new (&*slot) T(val);
            return;
        }

        int bucket = Traits<T>::hash(val) % m_buckets.size();
        m_buckets[bucket].prepend(val);
        if (++m_size >= m_buckets.capacity() * 2) {
            resize(m_buckets.capacity() * 4);
        }
    }

    void put(T&& val) {
        if (m_buckets.size() == 0) {
            for (int i = 0; i < m_buckets.capacity(); i++) {
                m_buckets.add(LinkedList<T>());
            }
        }

        auto slot = get(val);
        if (slot) {
            slot->~T();
            new (&*slot) T(move(val));
            return;
        }

        int bucket = Traits<T>::hash(val) % m_buckets.size();
        m_buckets[bucket].prepend(move(val));
        if (++m_size >= m_buckets.capacity() * 2) {
            resize(m_buckets.capacity() * 4);
        }
    }

    void toggle(const T& val) {
        if (get(val)) {
            remove(val);
            return;
        }

        put(val);
    }

    template<Hashable U>
    Maybe<T&> get(const U& key) {
        if (m_buckets.size() == 0) {
            return {};
        }

        int bucket = Traits<U>::hash(key) % m_buckets.size();
        T* val = nullptr;
        m_buckets[bucket].for_each([&](auto& obj) {
            if (obj == key) {
                val = &obj;
            }
        });
        return val;
    }

    template<Hashable U>
    Maybe<const T&> get(const U& key) const {
        return const_cast<HashSet<T>&>(*this).get(key);
    }

    template<Hashable U>
    void remove(const U& key) {
        int bucket = Traits<U>::hash(key) % m_buckets.size();
        if (m_buckets[bucket].remove_if([&](const auto& obj) -> bool {
                return obj == key;
            })) {
            m_size--;
        }
    }

    template<typename C>
    void for_each(C callback) const {
        m_buckets.for_each([&](auto& list) {
            list.for_each([&](auto& obj) {
                callback(obj);
            });
        });
    }

    int size() const { return m_size; }
    bool empty() const { return size() == 0; };

    bool operator==(const HashSet& other) const {
        if (this->size() != other.size()) {
            return false;
        }

        bool equal = true;
        this->for_each([&](auto& val) {
            if (!other.get(val) || *this->get(val) != *other.get(val)) {
                equal = false;
            }
        });
        return equal;
    }
    bool operator!=(const HashSet& other) const { return !(*this == other); }

    void resize(int new_capacity) {
        HashSet<T> temp(new_capacity);

        for_each([&](auto&& val) {
            temp.put(move(val));
        });

        swap(temp);
    }

    void swap(HashSet<T>& other) {
        LIIM::swap(this->m_buckets, other.m_buckets);
        LIIM::swap(this->m_size, other.m_size);
    }

private:
    Vector<LinkedList<T>> m_buckets;
    int m_size { 0 };
};

template<Hashable T>
void swap(HashSet<T>& a, HashSet<T>& b) {
    a.swap(b);
}
}

using LIIM::HashSet;
