#pragma once

#include <ext/forward.h>
#include <liim/format.h>
#include <liim/forward.h>
#include <liim/result.h>
#include <liim/string.h>
#include <liim/utilities.h>
#include <liim/variant.h>
#include <liim/vector.h>

namespace Ext {
template<typename T>
concept ClassErrorType = requires(const T& x) {
    { x.to_message() } -> LIIM::SameAs<String>;
};

template<typename T>
concept FreeErrorType = requires(const T& x) {
    { to_message(x) } -> LIIM::SameAs<String>;
};

template<ClassErrorType T>
String to_message_impl(const T& error) {
    return error.to_message();
}

template<FreeErrorType T>
String to_message_impl(const T& error) {
    return to_message(error);
}

template<typename T>
concept ErrorType = ClassErrorType<T> || FreeErrorType<T>;

class StringError {
public:
    StringError(String message) : m_message(move(message)) {}

    String to_message() const { return m_message; }

private:
    String m_message;
};

template<typename T, typename C, typename E = LIIM::InvokeResult<C, const T&>::type::ErrorType>
Result<Monostate, Vector<E>> collect_errors(const Vector<T>& input, C mapper) {
    Vector<E> output;
    for (auto& t : input) {
        auto result = mapper(t);
        if (result.is_error()) {
            output.add(move(result.error()));
        }
    }
    if (!output.empty()) {
        return Err(move(output));
    }
    return Ok(Monostate {});
}
}

namespace LIIM {
template<Ext::ErrorType T>
String to_message(const Vector<T>& errors) {
    Vector<String> messages;
    for (auto& error : errors) {
        messages.add(Ext::to_message_impl(error));
    }
    return String::join(move(messages), '\n');
}

template<typename... Types>
String to_message(const Variant<Types...> error) {
    return error.visit([](auto&& error) {
        return Ext::to_message_impl(error);
    });
}
}

namespace LIIM::Format {
template<Ext::ErrorType T>
struct Formatter<T> : Formatter<StringView> {
    void format(const T& error, FormatContext& context) { return format_string_view(Ext::to_message_impl(error).view(), context); }
};
}
