#pragma once

#include <liim/linked_list.h>
#include <liim/traits.h>
#include <liim/utilities.h>
#include <liim/vector.h>

namespace LIIM {

template<typename K, typename V> struct HashMapObj {
    HashMapObj(const K& key, const V& val) : m_key(key), m_value(val) {}
    HashMapObj(const K& key, V&& val) : m_key(key), m_value(val) {}

    ~HashMapObj() {}

    HashMapObj(const HashMapObj& other) : HashMapObj(other.m_key, other.m_value) {}

    K m_key;
    V m_value;
};

template<typename K, typename V> class HashMap {
public:
    explicit HashMap(int num_buckets = 20) : m_buckets(Vector<LinkedList<HashMapObj<K, V>>>(num_buckets)) {}

    HashMap(const HashMap<K, V>& other) : m_buckets(Vector<LinkedList<HashMapObj<K, V>>>(other.m_buckets.capacity())) {
        other.for_each_key([&](auto& key) {
            this->put(key, *other.get(key));
        });
    }

    HashMap(HashMap<K, V>&& other) : m_buckets(other.m_buckets), m_size(other.size()) {
        other.m_buckets.clear();
        other.m_size = 0;
    }

    ~HashMap() {}

    void clear() {
        m_buckets.for_each([&](auto& l) {
            l.clear();
        });
    }

    HashMap<K, V>& operator=(const HashMap<K, V>& other) {
        if (this != &other) {
            HashMap<K, V> temp(other);
            swap(temp);
        }

        return *this;
    }

    HashMap<K, V>& operator=(HashMap<K, V>&& other) {
        if (this != &other) {
            HashMap<K, V> temp(other);
            swap(temp);
        }

        return *this;
    }

    void put(const K& key, const V& val) {
        if (m_buckets.size() == 0) {
            for (int i = 0; i < m_buckets.capacity(); i++) {
                m_buckets.add(LinkedList<HashMapObj<K, V>>());
            }
        }

        V* slot = get(key);
        if (slot) {
            slot->~V();
            new (slot) V(val);
            return;
        }

        int bucket = Traits<K>::hash(key) % m_buckets.size();
        m_buckets[bucket].prepend(HashMapObj(key, val));
        if (++m_size >= m_buckets.capacity() * 2) {
            resize(m_buckets.capacity() * 4);
        }
    }

    void put(const K& key, V&& val) {
        if (m_buckets.size() == 0) {
            for (int i = 0; i < m_buckets.capacity(); i++) {
                m_buckets.add(LinkedList<HashMapObj<K, V>>());
            }
        }

        V* slot = get(key);
        if (slot) {
            slot->~V();
            new (slot) V(LIIM::move(val));
            return;
        }

        int bucket = Traits<K>::hash(key) % m_buckets.size();
        m_buckets[bucket].prepend(HashMapObj(key, val));
        if (++m_size >= m_buckets.capacity() * 2) {
            resize(m_buckets.capacity() * 4);
        }
    }

    V* get(const K& key) {
        if (m_buckets.size() == 0) {
            return nullptr;
        }

        int bucket = Traits<K>::hash(key) % m_buckets.size();
        V* val = nullptr;
        m_buckets[bucket].for_each([&](auto& obj) {
            if (obj.m_key == key) {
                val = &obj.m_value;
            }
        });

        return val;
    }

    const V* get(const K& key) const { return const_cast<HashMap<K, V>&>(*this).get(key); }

    V& get_or(const K& key, V& val) {
        V* res = get(key);
        if (res) {
            return *res;
        } else {
            return val;
        }
    }

    const V& get_or(const K& key, const V& val) const { return const_cast<HashMap<K, V>&>(*this).get_or(key, val); }

    void remove(const K& key) {
        int bucket = Traits<K>::hash(key) % m_buckets.size();
        m_buckets[bucket].remove_if([&](const auto& obj) -> bool {
            m_size--;
            return obj.m_key == key;
        });
    }

    template<typename C> void for_each(C callback) const {
        m_buckets.for_each([&](auto& list) {
            list.for_each([&](auto& obj) {
                callback(obj.m_value);
            });
        });
    }

    template<typename C> void for_each_key(C callback) const {
        m_buckets.for_each([&](const auto& list) {
            list.for_each([&](const auto& obj) {
                callback(obj.m_key);
            });
        });
    }

    int size() const { return m_size; }

    bool empty() const { return size() == 0; };

    bool operator==(const HashMap<K, V>& other) const {
        if (this->size() != other.size()) {
            return false;
        }

        bool equal = true;
        this->for_each_key([&](auto& key) {
            if (!other.get(key) || *this->get(key) != *other.get(key)) {
                equal = false;
            }
        });
        return equal;
    }

    void resize(int new_capacity) {
        HashMap<K, V> temp(new_capacity);

        for_each_key([&](const auto& key) {
            temp.put(key, *get(key));
        });

        swap(temp);
    }

    void swap(HashMap<K, V>& other) {
        LIIM::swap(this->m_buckets, other.m_buckets);
        LIIM::swap(this->m_size, other.m_size);
    }

private:
    Vector<LinkedList<HashMapObj<K, V>>> m_buckets;
    int m_size { 0 };
};

template<typename K, typename V> void swap(HashMap<K, V>& a, HashMap<K, V> b) {
    a.swap(b);
}

}

using LIIM::HashMap;