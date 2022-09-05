#pragma once

#include <di/container/vector/constant_vector.h>
#include <di/vocab/optional/prelude.h>
#include <di/vocab/span/span_fixed_size.h>

namespace di::container {
template<typename Self, typename Value, typename ConstValue>
class ConstantVectorInterface {
private:
    constexpr auto span() { return static_cast<Self&>(*this).span(); }
    constexpr auto span() const { return static_cast<Self const&>(*this).span(); }

public:
    constexpr types::size_t size() const { return span().size(); }
    constexpr types::size_t size_bytes() const { return sizeof(Value) * size(); }
    [[nodiscard]] constexpr bool empty() const { return size() == 0; }

    constexpr Value* begin() { return data(); }
    constexpr Value* end() { return data() + size(); }

    constexpr Value const* begin() const { return data(); }
    constexpr Value const* end() const { return data() + size(); }

    constexpr vocab::Optional<Value&> front() {
        if (empty()) {
            return nullopt;
        }
        return (*this)[0];
    }

    constexpr vocab::Optional<ConstValue&> front() const {
        if (empty()) {
            return nullopt;
        }
        return (*this)[0];
    }

    constexpr vocab::Optional<Value&> back() {
        if (empty()) {
            return nullopt;
        }
        return (*this)[size() - 1];
    }

    constexpr vocab::Optional<ConstValue&> back() const {
        if (empty()) {
            return nullopt;
        }
        return (*this)[size() - 1];
    }

    constexpr vocab::Optional<Value&> at(types::size_t index) {
        if (index >= size()) {
            return nullopt;
        }
        return (*this)[index];
    }

    constexpr vocab::Optional<ConstValue&> at(types::size_t index) const {
        if (index >= size()) {
            return nullopt;
        }
        return (*this)[index];
    }

    constexpr Value& operator[](types::size_t index) {
        // DI_ASSERT( index < size() )
        return data()[index];
    }

    constexpr ConstValue& operator[](types::size_t index) const {
        // DI_ASSERT( index < size() )
        return data()[index];
    }

    constexpr Value* data() { return span().data(); }
    constexpr ConstValue* data() const { return span().data(); }

    constexpr auto first(types::size_t count) {
        using Result = vocab::Optional<vocab::Span<Value>>;
        if (count > size()) {
            return Result(nullopt);
        }
        return Result(vocab::Span { data(), count });
    }

    constexpr auto first(types::size_t count) const {
        using Result = vocab::Optional<vocab::Span<ConstValue>>;
        if (count > size()) {
            return Result(nullopt);
        }
        return Result(vocab::Span { data(), count });
    }

    constexpr auto last(types::size_t count) {
        using Result = vocab::Optional<vocab::Span<Value>>;
        if (count > size()) {
            return Result(nullopt);
        }
        return Result(vocab::Span { end() - count, count });
    }

    constexpr auto last(types::size_t count) const {
        using Result = vocab::Optional<vocab::Span<ConstValue>>;
        if (count > size()) {
            return Result(nullopt);
        }
        return Result(vocab::Span { end() - count, count });
    }

    constexpr auto subspan(types::size_t offset) {
        using Result = vocab::Optional<vocab::Span<Value>>;
        if (offset > size()) {
            return Result(nullopt);
        }
        return Result(vocab::Span { data() + offset, size() - offset });
    }

    constexpr auto subspan(types::size_t offset) const {
        using Result = vocab::Optional<vocab::Span<ConstValue>>;
        if (offset > size()) {
            return Result(nullopt);
        }
        return Result(vocab::Span { data() + offset, size() - offset });
    }

    constexpr auto subspan(types::size_t offset, types::size_t count) {
        using Result = vocab::Optional<vocab::Span<Value>>;
        if (offset + count > size()) {
            return Result(nullopt);
        }
        return Result(vocab::Span { data() + offset, count });
    }

    constexpr auto subspan(types::size_t offset, types::size_t count) const {
        using Result = vocab::Optional<vocab::Span<ConstValue>>;
        if (offset + count > size()) {
            return Result(nullopt);
        }
        return Result(vocab::Span { data() + offset, count });
    }

    template<types::size_t count>
    constexpr Optional<vocab::Span<Value, count>> first() {
        if (count > size()) {
            return nullopt;
        }
        return vocab::Span<Value, count> { data(), count };
    }

    template<types::size_t count>
    constexpr Optional<vocab::Span<ConstValue, count>> first() const {
        if (count > size()) {
            return nullopt;
        }
        return vocab::Span<Value, count> { data(), count };
    }

    template<types::size_t count>
    constexpr Optional<vocab::Span<Value, count>> last() {
        if (count > size()) {
            return nullopt;
        }
        return vocab::Span<Value, count> { end() - count, count };
    }

    template<types::size_t count>
    constexpr Optional<vocab::Span<ConstValue, count>> last() const {
        if (count > size()) {
            return nullopt;
        }
        return vocab::Span<Value, count> { end() - count, count };
    }

    template<types::size_t offset, types::size_t count = vocab::dynamic_extent>
    constexpr Optional<vocab::Span<Value, count>> subspan() {
        if constexpr (count == vocab::dynamic_extent) {
            if (offset > size()) {
                return nullopt;
            }
            return vocab::Span { data() + offset, end() };
        } else {
            if (offset + count > size()) {
                return nullopt;
            }
            return vocab::Span<Value, count> { data() + offset, count };
        }
    }

    template<types::size_t offset, types::size_t count = vocab::dynamic_extent>
    constexpr Optional<vocab::Span<ConstValue, count>> subspan() const {
        if constexpr (count == vocab::dynamic_extent) {
            if (offset > size()) {
                return nullopt;
            }
            return vocab::Span { data() + offset, end() };
        } else {
            if (offset + count > size()) {
                return nullopt;
            }
            return vocab::Span<Value, count> { data() + offset, count };
        }
    }
};
}
