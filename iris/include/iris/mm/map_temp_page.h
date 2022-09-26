#pragma once

#include <iris/core/error.h>

namespace iris::mm {
struct TempPage;

Expected<TempPage> map_temp_page();
void unmap_temp_page(void*);

struct TempPage {
public:
    explicit TempPage(void* data) : m_data(data) {}

    ~TempPage() { unmap_temp_page(m_data); }

    TempPage(TempPage const&) = delete;
    TempPage(TempPage&&) = delete;
    TempPage& operator=(TempPage const&) = delete;
    TempPage& operator=(TempPage&&) = delete;

    template<typename T>
    requires(sizeof(T) <= 4096 && alignof(T) <= 4096)
    T* typed() const {
        return reinterpret_cast<T*>(m_data);
    }

private:
    void* m_data;
};
}