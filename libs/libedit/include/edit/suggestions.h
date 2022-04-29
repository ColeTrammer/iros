#pragma once

#include <edit/forward.h>
#include <edit/text_range.h>
#include <liim/maybe.h>
#include <liim/string.h>
#include <liim/vector.h>

namespace Edit {
class Suggestion {
public:
    Suggestion(String content, const TextIndex& start) : m_content(move(content)), m_start(start) {}

    const TextIndex& start() const { return m_start; }
    const String& content() const { return m_content; }

private:
    String m_content;
    TextIndex m_start;
};

class MatchedSuggestion {
public:
    explicit MatchedSuggestion(const Suggestion& suggestion, Vector<size_t> detailed_match, int score)
        : m_content(suggestion.content().view()), m_start(suggestion.start()), m_detailed_match(move(detailed_match)), m_score(score) {}

    const TextIndex& start() const { return m_start; }
    const String& content() const { return m_content; }
    const Vector<size_t>& detailed_match() const { return m_detailed_match; }
    int score() const { return m_score; }

private:
    String m_content;
    TextIndex m_start;
    Vector<size_t> m_detailed_match;
    int m_score { 0 };
};

class Suggestions {
public:
    auto begin() const { return m_matched_suggestions.begin(); }
    auto end() const { return m_matched_suggestions.end(); }

    int size() const { return m_matched_suggestions.size(); }
    bool empty() const { return m_matched_suggestions.empty(); }

    const MatchedSuggestion& first() const { return m_matched_suggestions.first(); }
    const MatchedSuggestion& last() const { return m_matched_suggestions.last(); }

    const MatchedSuggestion& operator[](int index) const { return m_matched_suggestions[index]; }

    void clear() {
        m_suggestions.clear();
        m_matched_suggestions.clear();
        m_current_text_range = {};
    }

    void compute_matches(const Document& document, const Cursor& cursor);
    void set_suggestions(Vector<Suggestion> suggestions) {
        m_suggestions = move(suggestions);
        m_matched_suggestions.clear();
    }

    const Maybe<TextRange>& current_text_range() const { return m_current_text_range; }
    void set_current_text_range(Maybe<TextRange> text_range) { m_current_text_range = move(text_range); }

private:
    void do_match(const Suggestion& suggestion, StringView reference_text);

    Vector<Suggestion> m_suggestions;
    Vector<MatchedSuggestion> m_matched_suggestions;
    Maybe<TextRange> m_current_text_range;
};
}
