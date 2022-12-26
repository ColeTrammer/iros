#pragma once

#include <di/container/algorithm/all_of.h>
#include <di/container/algorithm/compare.h>
#include <di/container/algorithm/equal.h>
#include <di/container/path/path_iterator.h>
#include <di/container/string/string_view.h>
#include <di/vocab/optional/prelude.h>
#include <di/vocab/tuple/prelude.h>

namespace di::container {
template<concepts::Encoding Enc>
class PathViewImpl
    : public meta::EnableView<PathViewImpl<Enc>>
    , public meta::EnableBorrowedContainer<PathViewImpl<Enc>> {
private:
    using View = string::StringViewImpl<Enc>;
    using CodePoint = meta::EncodingCodePoint<Enc>;
    using ViewIter = meta::ContainerIterator<View>;

public:
    PathViewImpl() = default;

    constexpr PathViewImpl(View view) : m_view(view) {
        if (view.starts_with(CodePoint('/'))) {
            m_first_component_end = container::next(m_view.begin());
        } else {
            m_first_component_end = container::find(m_view, CodePoint('/'));
        }
    }

    constexpr bool empty() const { return m_view.empty(); }
    constexpr auto data() const { return m_view; }

    constexpr auto begin() const { return PathIterator(m_view, { m_view.begin(), m_first_component_end }); }
    constexpr auto end() const { return PathIterator(m_view, { m_view.end(), m_view.end() }); }

    constexpr bool is_absolute() const { return m_view.starts_with(CodePoint('/')); }
    constexpr bool is_relative() const { return !is_absolute(); }

    constexpr Optional<View> filename() const {
        return lift_bool(!empty() && !m_view.ends_with(CodePoint('/'))) % [&] {
            auto trailing_slash = m_view.rfind(CodePoint('/'));
            if (!trailing_slash) {
                return data();
            }
            return m_view.substr(trailing_slash.end());
        };
    }

    constexpr Optional<View> extension() const {
        auto filename = this->filename();
        if (!filename) {
            return nullopt;
        }
        auto split = split_filename(*filename);
        return util::get<1>(split);
    }

    constexpr Optional<View> stem() const {
        auto filename = this->filename();
        if (!filename) {
            return nullopt;
        }
        auto split = split_filename(*filename);
        return util::get<0>(split);
    }

    constexpr Optional<PathViewImpl> parent_path() const {
        return lift_bool(!empty() && !container::all_of(m_view,
                                                        [](auto c) {
                                                            return c == CodePoint('/');
                                                        })) >>
               [&] {
                   auto result = auto(*this);
                   result.strip_filename();
                   return lift_bool(!result.empty()) % [&] {
                       return result;
                   };
               };
    }

    constexpr bool starts_with(PathViewImpl prefix) const { return container::starts_with(*this, prefix); }
    constexpr bool ends_with(PathViewImpl suffix) const { return container::ends_with(*this, suffix); }

    constexpr bool filename_ends_with(View suffix) const {
        return filename() % [&](auto filename) {
            return filename.ends_with(suffix);
        } == true;
    }

private:
    constexpr friend bool operator==(PathViewImpl const& a, PathViewImpl const& b) { return container::equal(a, b); }
    constexpr friend auto operator<=>(PathViewImpl const& a, PathViewImpl const& b) { return container::compare(a, b); }

    constexpr static Tuple<Optional<View>, Optional<View>> split_filename(View filename) {
        auto last_dot_view = filename.rfind(CodePoint('.'));
        if (!last_dot_view || last_dot_view.begin() == filename.begin()) {
            return { filename, nullopt };
        }
        return { filename.substr(filename.begin(), last_dot_view.begin()), filename.substr(last_dot_view.end()) };
    }

    constexpr void strip_filename() {
        while (m_view.ends_with(CodePoint('/'))) {
            m_view.replace_end(container::prev(m_view.end()));
        }
        while (!m_view.empty() && !m_view.ends_with(CodePoint('/'))) {
            m_view.replace_end(container::prev(m_view.end()));
        }
        while (m_view.size() > 2 && m_view.ends_with(CodePoint('/'))) {
            m_view.replace_end(container::prev(m_view.end()));
        }
    }

    View m_view;
    ViewIter m_first_component_end;
};
}