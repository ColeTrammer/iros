#pragma once

#include <liim/hash_set.h>

namespace LIIM {

template<Hashable K, typename V>
struct HashMapObj {
    bool operator==(const HashMapObj& other) const { return this->m_key == other.m_key; }
    bool operator==(const K& key) const { return this->m_key == key; }
    bool operator!=(const HashMapObj& other) const { return this->m_key != other.m_key; }
    bool operator!=(const K& key) const { return this->m_key != key; }

    K m_key;
    V m_value;
};

template<Hashable K, typename V>
struct Traits<HashMapObj<K, V>> {
    static constexpr bool is_simple() { return false; }
    static unsigned int hash(const HashMapObj<K, V>& obj) { return Traits<K>::hash(obj.m_key); };
};

template<Hashable K, typename V>
class HashMap {
public:
    explicit HashMap(int num_buckets = 20) : m_set(num_buckets) {}

    void clear() { m_set.clear(); }

    void put(const K& key, const V& val) { m_set.put({ key, val }); }
    void put(const K& key, V&& val) { m_set.put({ key, move(val) }); }

    V* get(const K& key) {
        auto* obj = m_set.get(key);
        if (obj) {
            return &obj->m_value;
        }
        return nullptr;
    }
    const V* get(const K& key) const {
        auto* obj = m_set.get(key);
        if (obj) {
            return &obj->m_value;
        }
        return nullptr;
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

    void remove(const K& key) { m_set.remove(key); }

    template<typename C>
    void for_each(C callback) const {
        m_set.for_each([&](auto& obj) {
            callback(obj.m_value);
        });
    }

    template<typename C>
    void for_each_key(C callback) const {
        m_set.for_each([&](auto& obj) {
            callback(obj.m_key);
        });
    }

    int size() const { return m_set.size(); }
    bool empty() const { return m_set.empty(); };

    bool operator==(const HashMap<K, V>& other) const { return m_set == other.m_set; }
    bool operator!=(const HashMap<K, V>& other) const { return m_set != other.m_set; }

    void resize(int new_capacity) { m_set.resize(new_capacity); }

    void swap(HashMap<K, V>& other) { LIIM::swap(this->m_set, other.m_set); }

private:
    HashSet<HashMapObj<K, V>> m_set;
};

template<Hashable K, typename V>
void swap(HashMap<K, V>& a, HashMap<K, V> b) {
    a.swap(b);
}

}

using LIIM::HashMap;
