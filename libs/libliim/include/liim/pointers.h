#pragma once

#include <assert.h>
#include <stdio.h>

namespace LIIM {

template<typename T> class UniquePtr {
public:
    explicit UniquePtr(T* ptr = nullptr)
        : m_ptr(ptr) {
    }

    UniquePtr(const UniquePtr& other) = delete;

    ~UniquePtr() {
        delete m_ptr;
    }

    T& operator=(const UniquePtr& other) = delete;

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

    bool operator!() {
        return !m_ptr;
    }
    operator bool() {
        return !!m_ptr;
    }

private:
    T* m_ptr;
};

template<typename T> class SharedPtrControlBlock {
public:
    explicit SharedPtrControlBlock(T* ptr)
        : m_ptr(ptr)
        , m_ref_count(1) {
        assert(m_ptr);
    }

    ~SharedPtrControlBlock() {
        assert(m_ref_count == 0);
        delete m_ptr;
    }

    int ref_count() const {
        return m_ref_count;
    }

    void ref() {
        m_ref_count++;
    }

    void deref() {
        m_ref_count--;
    }

    T* ptr() {
        return m_ptr;
    }
    const T* ptr() const {
        return m_ptr;
    }

private:
    int m_ref_count;
    T* m_ptr;
};

template<typename T> class SharedPtr {
public:
    explicit SharedPtr(T* ptr)
        : m_control_block(new SharedPtrControlBlock(ptr)) {
    }

    SharedPtr() {
    }

    ~SharedPtr() {
        assert(m_control_block);
        m_control_block->deref();
        if (m_control_block->ref_count() == 0) {
            fprintf(stderr, "destroying shared ptr\n");
            delete m_control_block;
        }
        m_control_block = nullptr;
    }

    SharedPtr(const SharedPtr& other) {
        m_control_block = other.m_control_block;
        if (m_control_block) {
            m_control_block->ref();
        }
    }

    SharedPtr& operator=(const SharedPtr& other) {
        m_control_block = other.m_control_block;
        if (m_control_block) {
            m_control_block->ref();
        }
    }

    T* ptr() {
        if (!m_control_block) {
            return nullptr;
        }
        return m_control_block->ptr();
    }

    const T* ptr() const {
        if (!m_control_block) {
            return nullptr;
        }
        return m_control_block->ptr();
    }

    T& operator*() {
        assert(ptr());
        return *ptr();
    }

    const T& operator*() const {
        assert(ptr());
        return *ptr();
    }

    T* operator->() {
        assert(ptr());
        return ptr();
    }

    const T* operator->() const {
        assert(ptr());
        return ptr();
    }

    bool operator!() {
        return !ptr();
    }
    operator bool() {
        return !!ptr();
    }

private:
    SharedPtrControlBlock<T>* m_control_block { nullptr };
};

}

using LIIM::SharedPtr;
using LIIM::UniquePtr;