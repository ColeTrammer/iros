#pragma once

#include <liim/linked_list.h>
#include <liim/traits.h>
#include <liim/vector.h>
#include <new>

namespace LIIM {

template<typename K, typename V> struct HashMapObj {
    HashMapObj(const K& key, const V& val) : m_key(key), m_value(val) {}

    ~HashMapObj() {}

    HashMapObj(const HashMapObj& other) : HashMapObj(other.m_key, other.m_value) {}

    K m_key;
    V m_value;
};

template<typename K, typename V> class HashMap {
public:
    explicit HashMap(int num_buckets = 20) : m_buckets(Vector<LinkedList<HashMapObj<K, V>>>(num_buckets)) {
        for (int i = 0; i < num_buckets; i++) {
            m_buckets.add(LinkedList<HashMapObj<K, V>>());
        }
    }

    HashMap(const HashMap<K, V>& other) : m_buckets(Vector<LinkedList<HashMapObj<K, V>>>(other.m_buckets.size())) {
        for (int i = 0; i < other.m_buckets.size(); i++) {
            m_buckets.add(LinkedList<HashMapObj<K, V>>());
        }

        other.for_each_key([&](auto& key) {
            this->put(key, *other.get(key));
        });
    }

    ~HashMap() {}

    void put(const K& key, const V& val) {
        int bucket = Traits<K>::hash(key) % m_buckets.size();
        m_buckets[bucket].prepend(HashMapObj(key, val));
    }

    V* get(const K& key) {
        int bucket = Traits<K>::hash(key) % m_buckets.size();
        V* val = nullptr;
        m_buckets[bucket].for_each([&](auto& obj) {
            if (obj.m_key == key) {
                val = &obj.m_value;
            }
        });

        return val;
    }

    const V* get(const K& key) const {
        int bucket = Traits<K>::hash(key) % m_buckets.size();
        V* val = nullptr;
        m_buckets[bucket].for_each([&](auto& obj) {
            if (obj.m_key == key) {
                val = &obj.m_value;
            }
        });

        return val;
    }

    V& get_or(const K& key, V& val) {
        V* res = get(key);
        if (res) {
            return *res;
        } else {
            return val;
        }
    }

    const V& get_or(const K& key, const V& val) const {
        const V* res = get(key);
        if (res) {
            return *res;
        } else {
            return val;
        }
    }

    void remove(const K& key) {
        int bucket = Traits<K>::hash(key) % m_buckets.size();
        m_buckets[bucket].remove_if([&](const auto& obj) -> bool {
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

    int size() const {
        int count = 0;
        for_each([&](auto&) {
            count++;
        });

        return count;
    }

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

private:
    Vector<LinkedList<HashMapObj<K, V>>> m_buckets;
};

}

using LIIM::HashMap;