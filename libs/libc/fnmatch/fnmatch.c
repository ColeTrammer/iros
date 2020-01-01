#include <assert.h>
#include <fnmatch.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

// Determines whether a character is in a given set
static bool fnmatch_is_valid_char_for_set(char c, const char *set, int set_end, bool invert) {
    for (int i = 0; i < set_end; i++) {
        // Handle `-` ranges
        if (i != 0 && i != set_end - 1 && set[i] == '-') {
            char range_start = set[i - 1];
            char range_end = set[i + 1];

            // Switch ranges so they work correctly if specified backwards
            if (range_start > range_end) {
                char t = range_end;
                range_end = range_start;
                range_start = t;
            }

            // Don't need to check edges b/c they are checked automatically
            for (char r = range_start + 1; r < range_end; r++) {
                if (r == c) {
                    return !invert;
                }
            }
        }

        if (set[i] == c) {
            return !invert;
        }
    }

    return invert;
}

int fnmatch(const char *pattern, const char *s, int flags) {
    assert(!(flags & FNM_PATHNAME));

    bool prev_was_backslash = false;

    size_t si = 0;
    size_t pi = 0;

    // Avoid recursion by using greedy matching (https://research.swtch.com/glob)
    size_t next_si = 0;
    size_t next_pi = 0;
    while (s[si] != '\0' && pattern[pi] != '\0') {
        if (pattern[pi] == '*' && !prev_was_backslash) {
            // We're not allowed to match dots
            if ((flags & FNM_PERIOD) && si == 0 && s[si] == '.') {
                return FNM_NOMATCH;
            }

            // Start by matching nothing, but we will continually try again with higher values of si until it matches
            next_pi = pi;
            next_si = si + 1;
            pi++;

            // If the last in the pattern character is a `*` - simply return 0
            if (pattern[pi] == '\0') {
                return 0;
            }
        } else if (pattern[pi] == '?' && !prev_was_backslash) {
            if ((flags & FNM_PERIOD) && si == 0 && s[si] == '.') {
                return FNM_NOMATCH;
            }
            si++;
            pi++;
        } else if (pattern[pi] == '[' && !prev_was_backslash) {
            pi++;
            bool invert = false;
            if (pattern[pi] == '!') {
                invert = true;
                pi++;
            }
            const char *set_start = pattern + pi;
            // Sets cannot be empty
            pi++;
            while (pattern[pi] != ']') {
                pi++;
            }

            // Refuse to match . if FNM_PERIOD is set, and we're using a
            // an inverted match. It's not specified whether or not it
            // should match, so reject it.
            if ((flags & FNM_PERIOD) && invert && si == 0 && s[si] == '.') {
                return FNM_NOMATCH;
            }

            if (!fnmatch_is_valid_char_for_set(s[si++], set_start, pi++, invert)) {
                // Try again if we were trying to match a `*`
                if (next_si != 0) {
                    si = next_si;
                    pi = next_pi;
                } else {
                    return FNM_NOMATCH;
                }
            }
        } else if (pattern[pi] == '\\' && !prev_was_backslash && !(flags & FNM_NOESCAPE)) {
            prev_was_backslash = true;
            continue;
        } else {
            if (s[si++] != pattern[pi++]) {
                // Try again if we were trying to match a `*`
                if (next_si != 0) {
                    si = next_si;
                    pi = next_pi;
                } else {
                    return FNM_NOMATCH;
                }
            }
        }
        prev_was_backslash = false;
    }

    return s[si] == '\0' && pattern[pi] == '\0' ? 0 : FNM_NOMATCH;
}