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
class PathViewImpl;

template<typename Self, concepts::Encoding Enc>
class ConstantPathInterface {
private:
    using View = string::StringViewImpl<Enc>;
    using CodePoint = meta::EncodingCodePoint<Enc>;
    using ViewIter = meta::ContainerIterator<View>;
    using PathView = PathViewImpl<Enc>;

    constexpr Self& self() { return static_cast<Self&>(*this); }
    constexpr Self const& self() const { return static_cast<Self const&>(*this); }

    constexpr View data() const { return self().data(); }

public:
    constexpr bool empty() const { return data().empty(); }

    constexpr PathView view() const& { return PathView(data()); }
    constexpr operator PathView() const& { return view(); }

    constexpr auto begin() const { return PathIterator(data(), { data().begin(), m_first_component_end }); }
    constexpr auto end() const { return PathIterator(data(), { data().end(), data().end() }); }

    constexpr bool is_absolute() const { return data().starts_with(CodePoint('/')); }
    constexpr bool is_relative() const { return !is_absolute(); }

    constexpr Optional<View> filename() const {
        return lift_bool(!empty() && !data().ends_with(CodePoint('/'))) % [&] {
            auto trailing_slash = data().rfind(CodePoint('/'));
            if (!trailing_slash) {
                return data();
            }
            return data().substr(trailing_slash.end());
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

    constexpr Optional<PathView> parent_path() const {
        return lift_bool(!empty() && !container::all_of(data(), function::equal(CodePoint('/')))) >> [&] {
            auto result = PathView(strip_filename(data()));
            return lift_bool(!result.empty()) % [&] {
                return result;
            };
        };
    }

    constexpr bool starts_with(PathView prefix) const { return container::starts_with(*this, prefix); }
    constexpr bool ends_with(PathView suffix) const { return container::ends_with(*this, suffix); }

    constexpr bool filename_ends_with(View suffix) const {
        return filename() % [&](auto filename) {
            return filename.ends_with(suffix);
        } == true;
    }

    constexpr Optional<PathView> strip_prefix(PathView prefix) {
        auto [a, b] = container::mismatch(*this, prefix);
        if (b != prefix.end()) {
            return nullopt;
        }
        return PathView(View(a.current_data(), this->end().current_data()));
    }

protected:
    constexpr void compute_first_component_end() {
        if (data().starts_with(CodePoint('/'))) {
            m_first_component_end = container::next(data().begin());
        } else {
            m_first_component_end = container::find(data(), CodePoint('/'));
        }
    }

private:
    constexpr friend bool operator==(Self const& a, Self const& b) { return container::equal(a, b); }
    constexpr friend auto operator<=>(Self const& a, Self const& b) { return container::compare(a, b); }

    constexpr static Tuple<Optional<View>, Optional<View>> split_filename(View filename) {
        auto last_dot_view = filename.rfind(CodePoint('.'));
        if (!last_dot_view || last_dot_view.begin() == filename.begin()) {
            return { filename, nullopt };
        }
        return { filename.substr(filename.begin(), last_dot_view.begin()), filename.substr(last_dot_view.end()) };
    }

    constexpr static View strip_filename(View view) {
        while (view.ends_with(CodePoint('/'))) {
            view.replace_end(container::prev(view.end()));
        }
        while (!view.empty() && !view.ends_with(CodePoint('/'))) {
            view.replace_end(container::prev(view.end()));
        }
        while (view.size() > 2 && view.ends_with(CodePoint('/'))) {
            view.replace_end(container::prev(view.end()));
        }
        return view;
    }

    ViewIter m_first_component_end {};
};
}