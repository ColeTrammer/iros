#pragma once

#include <assert.h>
#include <liim/utilities.h>

#if __GNUC__ >= 12
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wuse-after-free"
#endif

#ifndef STD_POINTERS
namespace LIIM {

template<typename T>
class UniquePtr {
public:
    constexpr explicit UniquePtr(T* ptr = nullptr) : m_ptr(ptr) {}

    constexpr UniquePtr(std::nullptr_t) : m_ptr(nullptr) {}

    constexpr UniquePtr(UniquePtr&& other) : m_ptr(other.m_ptr) { other.m_ptr = nullptr; }
    template<typename U>
    constexpr UniquePtr(UniquePtr<U>&& other) : m_ptr(static_cast<T*>(other.m_ptr)) {
        other.m_ptr = nullptr;
    }

    constexpr ~UniquePtr() {
        auto ptr = m_ptr;
        m_ptr = nullptr;
        delete ptr;
    }

    constexpr UniquePtr& operator=(std::nullptr_t) {
        UniquePtr<T> temp;
        swap(temp);
        return *this;
    }

    constexpr UniquePtr& operator=(const UniquePtr& other) = delete;
    template<typename U>
    constexpr UniquePtr& operator=(const UniquePtr<U>& other) = delete;

    template<typename U>
    constexpr UniquePtr& operator=(UniquePtr<U>&& other) {
        UniquePtr<T> temp(move(other));
        swap(temp);
        return *this;
    }

    constexpr UniquePtr& operator=(UniquePtr&& other) {
        UniquePtr<T> temp(move(other));
        swap(temp);
        return *this;
    }

    constexpr void swap(UniquePtr& other) { LIIM::swap(m_ptr, other.m_ptr); }

    constexpr T& operator*() {
        assert(m_ptr);
        return *m_ptr;
    }

    constexpr const T& operator*() const {
        assert(m_ptr);
        return *m_ptr;
    }

    constexpr T* operator->() {
        assert(m_ptr);
        return m_ptr;
    }

    constexpr const T* operator->() const {
        assert(m_ptr);
        return m_ptr;
    }

    constexpr T* get() { return m_ptr; }
    constexpr const T* get() const { return m_ptr; }

    constexpr bool operator!() const { return !m_ptr; }
    constexpr operator bool() const { return !!m_ptr; }

    template<typename U>
    constexpr bool operator==(const UniquePtr<U>& other) const {
        return m_ptr == other.m_ptr;
    }
    constexpr bool operator==(const UniquePtr& other) { return m_ptr == other.m_ptr; }

    template<typename U>
    constexpr bool operator!=(const UniquePtr<U>& other) const {
        return m_ptr != other.m_ptr;
    }
    constexpr bool operator!=(const UniquePtr& other) { return m_ptr != other.m_ptr; }

private:
    template<typename U>
    friend class UniquePtr;

    T* m_ptr;
};

template<typename T>
constexpr void swap(UniquePtr<T>& a, UniquePtr<T>& b) {
    a.swap(b);
}

template<typename T>
struct IsTriviallyRelocatable<UniquePtr<T>> {
    constexpr static bool value = true;
};

template<typename T, class... Args>
constexpr UniquePtr<T> make_unique(Args&&... args) {
    return UniquePtr<T>(new T(forward<Args>(args)...));
}

class SharedPtrControlBlock {
public:
    ~SharedPtrControlBlock() { assert(m_ref_count == 0); }

    int ref_count() const { return m_ref_count; }
    void ref() { m_ref_count++; }
    void deref() { m_ref_count--; }

    int weak_ref_count() { return m_weak_ref_count; }
    void weak_ref() { m_weak_ref_count++; }
    void weak_deref() { m_weak_ref_count--; }

private:
    int m_ref_count { 1 };
    int m_weak_ref_count { 0 };
};

template<typename T>
class WeakPtr;

template<typename T>
class SharedPtr {
public:
    explicit SharedPtr(T* ptr) : m_ptr(ptr), m_control_block(new SharedPtrControlBlock) {}

    SharedPtr(std::nullptr_t) : m_ptr(nullptr), m_control_block(nullptr) {}

    SharedPtr() {}

    ~SharedPtr() {
        if (!m_control_block) {
            return;
        }

        auto ptr = m_ptr;
        auto control_block = m_control_block;

        m_control_block = nullptr;
        m_ptr = nullptr;

        control_block->deref();
        if (control_block->ref_count() == 0) {
            if (control_block->weak_ref_count() == 0) {
                delete control_block;
            }
            delete ptr;
        }
    }

    SharedPtr(const SharedPtr& other) : m_ptr(other.m_ptr), m_control_block(other.m_control_block) {
        if (m_control_block) {
            m_control_block->ref();
        }
    }

    template<typename U>
    SharedPtr(const SharedPtr<U>& other) : m_ptr(static_cast<T*>(other.m_ptr)), m_control_block(other.m_control_block) {
        if (m_control_block) {
            m_control_block->ref();
        }
    }

    SharedPtr(SharedPtr&& other) : m_ptr(other.m_ptr), m_control_block(other.m_control_block) {
        other.m_ptr = nullptr;
        other.m_control_block = nullptr;
    }
    template<typename U>
    SharedPtr(SharedPtr<U>&& other) : m_ptr(static_cast<T*>(other.m_ptr)), m_control_block(other.m_control_block) {
        other.m_ptr = nullptr;
        other.m_control_block = nullptr;
    }

    SharedPtr& operator=(const SharedPtr& other) {
        SharedPtr temp(other);
        swap(temp);
        return *this;
    }

    template<typename U>
    SharedPtr& operator=(const SharedPtr<U>& other) {
        SharedPtr temp(other);
        swap(temp);
        return *this;
    }

    SharedPtr& operator=(SharedPtr&& other) {
        SharedPtr temp(move(other));
        swap(temp);
        return *this;
    }

    template<typename U>
    SharedPtr& operator=(SharedPtr<U>&& other) {
        SharedPtr temp(move(other));
        swap(temp);
        return *this;
    }

    SharedPtr& operator=(std::nullptr_t) {
        SharedPtr temp;
        swap(temp);
        return *this;
    }

    int ref_count() const { return m_control_block ? m_control_block->ref_count() : 0; }

    T* get() {
        if (!m_control_block) {
            return nullptr;
        }
        return m_ptr;
    }

    const T* get() const {
        if (!m_control_block) {
            return nullptr;
        }
        return m_ptr;
    }

    T& operator*() {
        assert(get());
        return *get();
    }

    const T& operator*() const {
        assert(get());
        return *get();
    }

    T* operator->() {
        assert(get());
        return get();
    }

    const T* operator->() const {
        assert(get());
        return get();
    }

    bool operator!() const { return !get(); }
    operator bool() const { return !!get(); }

    bool operator==(const SharedPtr& other) const { return this->get() == other.get(); }
    bool operator!=(const SharedPtr& other) const { return !(*this == other); }

    void swap(SharedPtr& other) {
        LIIM::swap(this->m_control_block, other.m_control_block);
        LIIM::swap(this->m_ptr, other.m_ptr);
    }

private:
    template<typename U>
    friend class SharedPtr;

    template<typename U>
    friend class WeakPtr;
    SharedPtr(SharedPtrControlBlock* control_block, T* ptr) : m_ptr(ptr), m_control_block(control_block) {}

    T* m_ptr { nullptr };
    SharedPtrControlBlock* m_control_block { nullptr };
};

template<typename T>
void swap(SharedPtr<T>& a, SharedPtr<T>& b) {
    a.swap(b);
}

template<typename T, class... Args>
SharedPtr<T> make_shared(Args&&... args) {
    return SharedPtr<T>(new T(forward<Args>(args)...));
}

template<typename T>
class WeakPtr {
public:
    constexpr WeakPtr() {}

    WeakPtr(const WeakPtr& other) : m_control_block(other.m_control_block), m_ptr(other.m_ptr) {
        if (m_control_block) {
            m_control_block->weak_ref();
        }
    }

    template<typename U>
    WeakPtr(const WeakPtr<U>& other) : m_control_block(other.m_control_block), m_ptr(static_cast<T*>(other.m_ptr)) {
        if (m_control_block) {
            m_control_block->weak_ref();
        }
    }

    WeakPtr(WeakPtr&& other) : m_control_block(other.m_control_block), m_ptr(other.m_ptr) {
        other.m_control_block = nullptr;
        other.m_ptr = nullptr;
    }

    template<typename U>
    WeakPtr(WeakPtr<U>&& other) : m_control_block(other.m_control_block), m_ptr(static_cast<T*>(other.m_ptr)) {
        other.m_control_block = nullptr;
        other.m_ptr = nullptr;
    }

    template<typename U>
    WeakPtr(const SharedPtr<U>& s) : m_control_block(s.m_control_block), m_ptr(static_cast<T*>(s.m_ptr)) {
        if (m_control_block) {
            m_control_block->weak_ref();
        }
    }

    ~WeakPtr() { reset(); }

    WeakPtr& operator=(const WeakPtr& other) {
        WeakPtr temp(other);
        swap(temp);
        return *this;
    }

    template<typename U>
    WeakPtr& operator=(const WeakPtr<U>& other) {
        WeakPtr temp(other);
        swap(temp);
        return *this;
    }

    WeakPtr& operator=(WeakPtr&& other) {
        WeakPtr temp(move(other));
        swap(temp);
        return *this;
    }

    template<typename U>
    WeakPtr& operator=(WeakPtr<U>&& other) {
        WeakPtr temp(move(other));
        swap(temp);
        return *this;
    }

    template<typename U>
    WeakPtr& operator=(const SharedPtr<U>& other) {
        WeakPtr temp(other);
        swap(temp);
        return *this;
    }

    void reset() {
        auto control_block = m_control_block;

        m_ptr = nullptr;
        m_control_block = nullptr;

        if (control_block) {
            control_block->weak_deref();
            if (control_block->ref_count() == 0 && control_block->weak_ref_count() == 0) {
                delete control_block;
            }
        }
    }

    void swap(WeakPtr& other) {
        LIIM::swap(this->m_control_block, other.m_control_block);
        LIIM::swap(this->m_ptr, other.m_ptr);
    }

    int use_count() const {
        if (!m_control_block) {
            return 0;
        }
        return m_control_block->ref_count();
    }
    bool expired() const { return use_count() == 0; }

    SharedPtr<T> lock() const {
        if (!m_control_block || expired()) {
            return SharedPtr<T>();
        }
        m_control_block->ref();
        return SharedPtr<T>(m_control_block, m_ptr);
    }

private:
    template<typename U>
    friend class WeakPtr;

    SharedPtrControlBlock* m_control_block { nullptr };
    T* m_ptr { nullptr };
};

template<typename T>
void swap(WeakPtr<T>& a, WeakPtr<T>& b) {
    a.swap(b);
}

}

using LIIM::make_shared;
using LIIM::make_unique;
using LIIM::SharedPtr;
using LIIM::UniquePtr;
using LIIM::WeakPtr;
#else
#include <memory>
template<typename T>
using SharedPtr = std::shared_ptr<T>;
template<typename T>
using UniquePtr = std::unique_ptr<T>;
template<typename T>
using WeakPtr = std::weak_ptr<T>;
using std::make_shared;
using std::make_unique;
#endif

#if __GNUC__ >= 12
#pragma GCC diagnostic pop
#endif
