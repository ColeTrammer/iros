#pragma once

#include <di/container/string/constant_string_interface.h>
#include <di/container/string/string_view_impl.h>
#include <di/container/string/utf8_encoding.h>
#include <di/vocab/span/prelude.h>

namespace di::container {
class ErasedString : public string::ConstantStringInterface<ErasedString, string::Utf8Encoding> {
public:
    using Encoding = string::Utf8Encoding;

    constexpr auto encoding() const { return Encoding {}; }
    constexpr auto span() const { return m_data; }

protected:
    enum class ThunkOp { Move, Destroy };

    using ThunkFunction = void (*)(ErasedString* dest, ErasedString const* src, ThunkOp op);

    Span<c8 const> m_data {};
    void* m_state[3] {};
    ThunkFunction const m_thunk { nullptr };

    constexpr explicit ErasedString(ThunkFunction thunk) : m_thunk(thunk) {}

public:
    constexpr explicit ErasedString(Span<c8 const> data = {}, void* state0 = nullptr, void* state1 = nullptr,
                                    void* state2 = nullptr, ThunkFunction thunk = nullptr)
        : m_data(data), m_state(state0, state1, state2), m_thunk(thunk) {}

    constexpr ErasedString(string::StringViewImpl<string::Utf8Encoding> view) : ErasedString(view.span()) {}

    constexpr ErasedString(ErasedString&& other)
        : m_data(other.m_data), m_state(other.m_state[0], other.m_state[1], other.m_state[2]), m_thunk(other.m_thunk) {
        if (this->m_thunk) {
            m_thunk(this, &other, ThunkOp::Move);
        }
    }

    constexpr ErasedString& operator=(ErasedString&& other) {
        if (this != &other) {
            util::destroy_at(this);
            util::construct_at(this, util::move(other));
        }
        return *this;
    }

    constexpr ~ErasedString() {
        if (m_thunk) {
            m_thunk(this, this, ThunkOp::Destroy);
        }
        m_data = {};
    }
};
}