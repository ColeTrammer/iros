#pragma once

#include <assert.h>
#include <liim/utilities.h>

#ifndef STD_POINTERS
namespace LIIM {

template<typename T>
class UniquePtr {
public:
    explicit UniquePtr(T* ptr = nullptr) : m_ptr(ptr) {}

    UniquePtr(std::nullptr_t) : m_ptr(nullptr) {}

    UniquePtr(UniquePtr&& other) : m_ptr(other.m_ptr) { other.m_ptr = nullptr; }
    template<typename U>
    UniquePtr(UniquePtr<U>&& other) : m_ptr(static_cast<T*>(other.m_ptr)) {
        other.m_ptr = nullptr;
    }

    ~UniquePtr() { delete m_ptr; }

    UniquePtr& operator=(std::nullptr_t) {
        UniquePtr<T> temp;
        swap(temp);
        return *this;
    }

    UniquePtr& operator=(const UniquePtr& other) = delete;
    template<typename U>
    UniquePtr& operator=(const UniquePtr<U>& other) = delete;

    template<typename U>
    UniquePtr& operator=(UniquePtr<U>&& other) {
        UniquePtr<T> temp(move(other));
        swap(temp);
        return *this;
    }

    UniquePtr& operator=(UniquePtr&& other) {
        UniquePtr<T> temp(move(other));
        swap(temp);
        return *this;
    }

    void swap(UniquePtr& other) { LIIM::swap(m_ptr, other.m_ptr); }

    T& operator*() {
        assert(m_ptr);
        return *m_ptr;
    }

    const T& operator*() const {
        assert(m_ptr);
        return *m_ptr;
    }

    T* operator->() {
        assert(m_ptr);
        return m_ptr;
    }

    const T* operator->() const {
        assert(m_ptr);
        return m_ptr;
    }

    T* get() { return m_ptr; }
    const T* get() const { return m_ptr; }

    bool operator!() const { return !m_ptr; }
    operator bool() const { return !!m_ptr; }

private:
    template<typename U>
    friend class UniquePtr;

    T* m_ptr;
};

template<typename T>
void swap(UniquePtr<T>& a, UniquePtr<T>& b) {
    a.swap(b);
}

template<typename T, class... Args>
UniquePtr<T> make_unique(Args&&... args) {
    return UniquePtr<T>(new T(forward<Args>(args)...));
}

class SharedPtrControlBlock {
public:
    explicit SharedPtrControlBlock() : m_ref_count(1) {}

    ~SharedPtrControlBlock() { assert(m_ref_count == 0); }

    int ref_count() const { return m_ref_count; }

    void ref() { m_ref_count++; }

    void deref() { m_ref_count--; }

private:
    int m_ref_count;
};

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

        m_control_block->deref();
        if (m_control_block->ref_count() == 0) {
            delete m_control_block;
            delete m_ptr;
        }
        m_control_block = nullptr;
        m_ptr = nullptr;
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
}

using LIIM::make_shared;
using LIIM::make_unique;
using LIIM::SharedPtr;
using LIIM::UniquePtr;
#else
#include <memory>
template<typename T>
using SharedPtr = std::shared_ptr<T>;
template<typename T>
using UniquePtr = std::unique_ptr<T>;
using std::make_shared;
using std::make_unique;
#endif
