#pragma once

#include <assert.h>
#include <liim/utilities.h>
#include <stdio.h>

#ifndef STD_POINTERS
namespace LIIM {

template<typename T> class UniquePtr {
public:
    explicit UniquePtr(T* ptr = nullptr) : m_ptr(ptr) {}

    UniquePtr(const UniquePtr& other) = delete;

    UniquePtr(UniquePtr&& other) : m_ptr(other.m_ptr) { other.m_ptr = nullptr; }

    ~UniquePtr() { delete m_ptr; }

    UniquePtr& operator=(std::nullptr_t) {
        UniquePtr<T> temp;
        swap(temp);
        return *this;
    }

    UniquePtr& operator=(const UniquePtr& other) = delete;

    UniquePtr& operator=(UniquePtr&& other) {
        UniquePtr<T> temp(other);
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

    bool operator!() { return !m_ptr; }
    operator bool() { return !!m_ptr; }

private:
    T* m_ptr;
};

template<typename T> void swap(UniquePtr<T>& a, UniquePtr<T>& b) {
    a.swap(b);
}

template<typename T, class... Args> UniquePtr<T> make_unique(Args&&... args) {
    return UniquePtr<T>(new T(forward<Args>(args)...));
}

template<typename T> class SharedPtrControlBlock {
public:
    explicit SharedPtrControlBlock(T* ptr) : m_ref_count(1), m_ptr(ptr) { assert(m_ptr); }

    ~SharedPtrControlBlock() {
        assert(m_ref_count == 0);
        delete m_ptr;
        m_ptr = nullptr;
    }

    int ref_count() const { return m_ref_count; }

    void ref() { m_ref_count++; }

    void deref() { m_ref_count--; }

    T* ptr() { return m_ptr; }
    const T* ptr() const { return m_ptr; }

private:
    int m_ref_count;
    T* m_ptr;
};

template<typename T> class SharedPtr {
public:
    explicit SharedPtr(T* ptr) : m_control_block(new SharedPtrControlBlock<T>(ptr)) {}

    SharedPtr() {}

    ~SharedPtr() {
        if (!m_control_block) {
            return;
        }

        m_control_block->deref();
        if (m_control_block->ref_count() == 0) {
            delete m_control_block;
        }
        m_control_block = nullptr;
    }

    SharedPtr(const SharedPtr& other) : m_control_block(other.m_control_block) {
        if (m_control_block) {
            m_control_block->ref();
        }
    }

    SharedPtr(SharedPtr&& other) : m_control_block(other.m_control_block) { other.m_control_block = nullptr; }

    SharedPtr& operator=(const SharedPtr& other) {
        SharedPtr temp(other);
        swap(temp);
        return *this;
    }

    SharedPtr& operator=(SharedPtr&& other) {
        SharedPtr temp(other);
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
        return m_control_block->ptr();
    }

    const T* get() const {
        if (!m_control_block) {
            return nullptr;
        }
        return m_control_block->ptr();
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

    void swap(SharedPtr& other) { LIIM::swap(this->m_control_block, other.m_control_block); }

private:
    SharedPtrControlBlock<T>* m_control_block { nullptr };
};

template<typename T> void swap(SharedPtr<T>& a, SharedPtr<T>& b) {
    a.swap(b);
}

template<typename T, class... Args> SharedPtr<T> make_shared(Args&&... args) {
    return SharedPtr<T>(new T(forward<Args>(args)...));
}
}

using LIIM::make_shared;
using LIIM::make_unique;
using LIIM::SharedPtr;
using LIIM::UniquePtr;
#else
#include <memory>
template<typename T> using SharedPtr = std::shared_ptr<T>;
template<typename T> using UniquePtr = std::unique_ptr<T>;
using std::make_shared;
using std::make_unique;
#endif
