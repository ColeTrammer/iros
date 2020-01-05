#pragma once

#define ENUMERATE_OPTIONS                 \
    __ENUMERATE_OPTIONS(allexport, 'a')   \
    __ENUMERATE_OPTIONS(errexit, 'e')     \
    __ENUMERATE_OPTIONS(ignoreeof, '\x1') \
    __ENUMERATE_OPTIONS(monitor, 'm')     \
    __ENUMERATE_OPTIONS(noclobber, 'C')   \
    __ENUMERATE_OPTIONS(noglob, 'f')      \
    __ENUMERATE_OPTIONS(noexec, 'n')      \
    __ENUMERATE_OPTIONS(nolog, '\x2')     \
    __ENUMERATE_OPTIONS(notify, 'b')      \
    __ENUMERATE_OPTIONS(nounset, 'u')     \
    __ENUMERATE_OPTIONS(verbose, 'v')     \
    __ENUMERATE_OPTIONS(vi, '\x3')        \
    __ENUMERATE_OPTIONS(xtrace, 'x')      \
    __ENUMERATE_OPTIONS(interactive, 'i') \
    __ENUMERATE_OPTIONS(hashfunctions, 'h')

class ShState {
public:
    static ShState& the();

    int flags_for_wordexp() const;
    bool process_arg(const char* s);
    void process_option(const char* s, bool to_set);

    void dump() const;

#undef __ENUMERATE_OPTIONS
#define __ENUMERATE_OPTIONS(s, c)             \
    bool option_##s() const { return m_##s; } \
    void set_##s(bool _##s) { m_##s = _##s; }

    ENUMERATE_OPTIONS

private:
    ShState() {}
    ~ShState() {}

#undef __ENUMERATE_OPTIONS
#define __ENUMERATE_OPTIONS(s, c) bool m_##s { false };

    ENUMERATE_OPTIONS
};