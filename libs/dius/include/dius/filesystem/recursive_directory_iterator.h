#pragma once

#include "di/container/queue/prelude.h"
#include "dius/filesystem/directory_iterator.h"

namespace dius::filesystem {
class RecursiveDirectoryIterator
    : public di::container::IteratorBase<RecursiveDirectoryIterator, di::InputIteratorTag,
                                         di::Expected<DirectoryEntry, di::GenericCode>, i64>
    , public di::meta::EnableBorrowedContainer<RecursiveDirectoryIterator>
    , public di::meta::EnableView<RecursiveDirectoryIterator> {
public:
    static auto create(di::Path path, DirectoryOptions options = DirectoryOptions::None)
        -> di::Result<RecursiveDirectoryIterator> {
        auto result = RecursiveDirectoryIterator { options };
        auto base = DI_TRY(DirectoryIterator::create(di::move(path), options));
        if (base != DirectoryIterator {}) {
            result.m_stack.push(di::move(base));
            result.m_recursion_pending = true;
        }
        return result;
    }

    RecursiveDirectoryIterator() = default;

    RecursiveDirectoryIterator(RecursiveDirectoryIterator const&) = delete;
    RecursiveDirectoryIterator(RecursiveDirectoryIterator&&) = default;

    auto operator=(RecursiveDirectoryIterator const&) -> RecursiveDirectoryIterator& = delete;
    auto operator=(RecursiveDirectoryIterator&&) -> RecursiveDirectoryIterator& = default;

    auto operator*() const -> di::Expected<DirectoryEntry const&, di::GenericCode> { return **m_stack.top(); }

    auto begin() -> RecursiveDirectoryIterator { return di::move(*this); }
    auto end() const -> RecursiveDirectoryIterator { return {}; }

    constexpr auto options() const -> DirectoryOptions { return m_options; }
    constexpr auto depth() const -> i32 { return static_cast<i32>(m_stack.size()) - 1; }
    constexpr auto recursion_pending() const -> bool { return m_recursion_pending; }

    void advance_one() {
        auto& top = *m_stack.top();
        if (!*top) {
            m_stack.clear();
            return;
        }
        auto const& current = **top;

        // If recursion is pending and the current entry is a directory,
        // recurse by pushing to the stack.
        if (di::exchange(m_recursion_pending, true)) {
            auto is_directory_result = current.is_non_symlink_directory();
            if (!is_directory_result) {
                top.m_current = di::Unexpected(is_directory_result.error());
                return;
            }

            // If this is really a directory, then create a new directory iterator and push.
            if (*is_directory_result) {
                auto new_iterator = DirectoryIterator::create(current.path_view().to_owned(), options());
                if (!new_iterator) {
                    top.m_current = di::Unexpected(new_iterator.error());
                    return;
                }

                // If there are no entries, skip pushing.
                if (new_iterator != DirectoryIterator {}) {
                    m_stack.push(*di::move(new_iterator));
                    return;
                }
            }
        }

        // Advance the current directory iterator, and pop if there is nothing left.
        ++top;
        if (top == DirectoryIterator {}) {
            pop();
        }
    }

    void pop() {
        for (;;) {
            m_stack.pop();

            if (!m_stack.empty()) {
                auto& top = *m_stack.top();
                ++top;
                if (top == DirectoryIterator {}) {
                    continue;
                }
            }
            break;
        }
    }
    constexpr void disable_recursion_pending() { m_recursion_pending = false; }

private:
    constexpr explicit RecursiveDirectoryIterator(DirectoryOptions options) : m_options(options) {}

    constexpr friend auto operator==(RecursiveDirectoryIterator const& a, RecursiveDirectoryIterator const& b) -> bool {
        return a.m_stack.empty() && b.m_stack.empty();
    }

    di::Stack<DirectoryIterator> m_stack;
    DirectoryOptions m_options { DirectoryOptions::None };
    bool m_recursion_pending { false };
};
}
