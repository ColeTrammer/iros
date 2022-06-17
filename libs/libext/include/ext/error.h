#pragma once

#include <errno.h>
#include <ext/forward.h>
#include <liim/container/new_vector.h>
#include <liim/format.h>
#include <liim/forward.h>
#include <liim/result.h>
#include <liim/source_location.h>
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

class SystemError {
public:
    static SystemError from_error_code(int error_code, NewVector<String> arguments = {},
                                       SourceLocation location = SourceLocation::current()) {
        return SystemError(move(arguments), location.function_name(), error_code);
    }
    static SystemError from_errno(NewVector<String> arguments = {}, SourceLocation location = SourceLocation::current()) {
        return SystemError(move(arguments), location.function_name(), errno);
    }

    StringView system_call() const { return m_system_call; }
    int error_code() const { return m_error_code; }

    String to_message() const {
        String arguments;
        bool first = true;
        for (auto& argument : m_arguments) {
            if (!first) {
                arguments += ", ";
            }
            arguments += argument;
            first = false;
        }
        return format("{}({}) failed: {}", m_system_call, arguments, strerror(m_error_code));
    }

private:
    SystemError(NewVector<String> arguments, StringView system_call, int error_code)
        : m_arguments(move(arguments)), m_system_call(system_call), m_error_code(error_code) {}

    NewVector<String> m_arguments;
    StringView m_system_call;
    int m_error_code { 0 };
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

template<typename Collection, typename C,
         typename R = LIIM::InvokeResult<C, decltype(*LIIM::declval<const Collection&>().begin())>::type::ErrorType>
Result<Monostate, R> stop_on_error(const Collection& input, C mapper) {
    for (auto& t : input) {
        TRY(mapper(t));
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
String to_message(const Variant<Types...>& error) {
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
