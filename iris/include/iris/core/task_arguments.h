#pragma once

#include <di/container/string/prelude.h>
#include <di/container/vector/prelude.h>
#include <di/util/prelude.h>
#include <di/vocab/pointer/prelude.h>

namespace iris {
class TaskArguments : public di::IntrusiveRefCount<TaskArguments> {
public:
    explicit TaskArguments(di::Vector<di::TransparentString> arguments, di::Vector<di::TransparentString> enviornment)
        : m_arguments(di::move(arguments)), m_enviornment(di::move(enviornment)) {}

    di::Span<di::TransparentString const> arguments() const { return m_arguments.span(); }
    di::Span<di::TransparentString const> enviornment() const { return m_enviornment.span(); }

private:
    di::Vector<di::TransparentString> m_arguments;
    di::Vector<di::TransparentString> m_enviornment;
};
}
