#pragma once

#include <iris/core/error.h>

namespace iris::mm {
struct TempPage;

auto map_temp_page() -> Expected<TempPage>;
void unmap_temp_page(void*);

struct TempPage {
public:
    explicit TempPage(void* data) : m_data(data) {}

    ~TempPage() { unmap_temp_page(m_data); }

    TempPage(TempPage const&) = delete;
    TempPage(TempPage&&) = delete;
    auto operator=(TempPage const&) -> TempPage& = delete;
    auto operator=(TempPage&&) -> TempPage& = delete;

    template<typename T>
    requires(sizeof(T) <= 4096 && alignof(T) <= 4096)
    auto typed() const -> T* {
        return reinterpret_cast<T*>(m_data);
    }

private:
    void* m_data;
};
}
