#ifdef DIUS_USE_RUNTIME
#include <ccpp/bits/all.h>
#else
#include <ctype.h>
#include <stdio.h>
#endif

#include <dius/test/prelude.h>

namespace ctype_h {
[[gnu::noinline]] static bool do_ctype(int (*f)(int), int ch) {
    return bool(di::black_box(f)(di::black_box(ch)));
}

[[gnu::noinline]] static int do_toc(int (*f)(int), int ch) {
    return di::black_box(f)(di::black_box(ch));
}

static void isalnum_() {
    ASSERT(do_ctype(isalnum, 'a'));
    ASSERT(do_ctype(isalnum, 'A'));
    ASSERT(do_ctype(isalnum, 'z'));
    ASSERT(do_ctype(isalnum, 'Z'));
    ASSERT(do_ctype(isalnum, 'x'));
    ASSERT(do_ctype(isalnum, 'X'));
    ASSERT(do_ctype(isalnum, '8'));
    ASSERT(do_ctype(isalnum, '0'));
    ASSERT(do_ctype(isalnum, '9'));
    ASSERT(!do_ctype(isalnum, '('));
    ASSERT(!do_ctype(isalnum, '{'));
    ASSERT(!do_ctype(isalnum, '\t'));
}

static void isalpha_() {
    ASSERT(do_ctype(isalpha, 'a'));
    ASSERT(do_ctype(isalpha, 'A'));
    ASSERT(do_ctype(isalpha, 'z'));
    ASSERT(do_ctype(isalpha, 'Z'));
    ASSERT(do_ctype(isalpha, 'x'));
    ASSERT(do_ctype(isalpha, 'X'));
    ASSERT(!do_ctype(isalpha, '8'));
    ASSERT(!do_ctype(isalpha, '0'));
    ASSERT(!do_ctype(isalpha, '9'));
    ASSERT(!do_ctype(isalpha, '('));
    ASSERT(!do_ctype(isalpha, '{'));
    ASSERT(!do_ctype(isalpha, '\t'));
    ASSERT(!do_ctype(isalpha, EOF));
}

static void isascii_() {
    ASSERT(do_ctype(isascii, 'a'));
    ASSERT(do_ctype(isascii, 'A'));
    ASSERT(do_ctype(isascii, 'z'));
    ASSERT(do_ctype(isascii, 'Z'));
    ASSERT(do_ctype(isascii, 'x'));
    ASSERT(do_ctype(isascii, 'X'));
    ASSERT(do_ctype(isascii, '8'));
    ASSERT(do_ctype(isascii, '0'));
    ASSERT(do_ctype(isascii, '9'));
    ASSERT(do_ctype(isascii, '('));
    ASSERT(do_ctype(isascii, '{'));
    ASSERT(do_ctype(isascii, '\t'));
    ASSERT(!do_ctype(isascii, 128));
    ASSERT(!do_ctype(isascii, 255));
    ASSERT(!do_ctype(isascii, 256));
    ASSERT(!do_ctype(isascii, EOF));
}

static void islower_() {
    ASSERT(do_ctype(islower, 'a'));
    ASSERT(!do_ctype(islower, 'A'));
    ASSERT(do_ctype(islower, 'z'));
    ASSERT(!do_ctype(islower, 'Z'));
    ASSERT(do_ctype(islower, 'x'));
    ASSERT(!do_ctype(islower, 'X'));
    ASSERT(!do_ctype(islower, '8'));
    ASSERT(!do_ctype(islower, '0'));
    ASSERT(!do_ctype(islower, '9'));
    ASSERT(!do_ctype(islower, '('));
    ASSERT(!do_ctype(islower, '{'));
    ASSERT(!do_ctype(islower, '\t'));
    ASSERT(!do_ctype(islower, EOF));
}

static void isupper_() {
    ASSERT(!do_ctype(isupper, 'a'));
    ASSERT(do_ctype(isupper, 'A'));
    ASSERT(!do_ctype(isupper, 'z'));
    ASSERT(do_ctype(isupper, 'Z'));
    ASSERT(!do_ctype(isupper, 'x'));
    ASSERT(do_ctype(isupper, 'X'));
    ASSERT(!do_ctype(isupper, '8'));
    ASSERT(!do_ctype(isupper, '0'));
    ASSERT(!do_ctype(isupper, '9'));
    ASSERT(!do_ctype(isupper, '('));
    ASSERT(!do_ctype(isupper, '{'));
    ASSERT(!do_ctype(isupper, '\t'));
    ASSERT(!do_ctype(isupper, EOF));
}

static void isdigit_() {
    ASSERT(!do_ctype(isdigit, 'a'));
    ASSERT(!do_ctype(isdigit, 'A'));
    ASSERT(!do_ctype(isdigit, 'z'));
    ASSERT(!do_ctype(isdigit, 'Z'));
    ASSERT(!do_ctype(isdigit, 'x'));
    ASSERT(!do_ctype(isdigit, 'X'));
    ASSERT(do_ctype(isdigit, '8'));
    ASSERT(do_ctype(isdigit, '0'));
    ASSERT(do_ctype(isdigit, '9'));
    ASSERT(!do_ctype(isdigit, '('));
    ASSERT(!do_ctype(isdigit, '{'));
    ASSERT(!do_ctype(isdigit, '\t'));
    ASSERT(!do_ctype(isdigit, EOF));
}

static void isxdigit_() {
    ASSERT(do_ctype(isxdigit, 'a'));
    ASSERT(do_ctype(isxdigit, 'A'));
    ASSERT(do_ctype(isxdigit, 'f'));
    ASSERT(do_ctype(isxdigit, 'F'));
    ASSERT(!do_ctype(isxdigit, 'z'));
    ASSERT(!do_ctype(isxdigit, 'Z'));
    ASSERT(!do_ctype(isxdigit, 'x'));
    ASSERT(!do_ctype(isxdigit, 'X'));
    ASSERT(do_ctype(isxdigit, '8'));
    ASSERT(do_ctype(isxdigit, '0'));
    ASSERT(do_ctype(isxdigit, '9'));
    ASSERT(!do_ctype(isxdigit, '('));
    ASSERT(!do_ctype(isxdigit, '{'));
    ASSERT(!do_ctype(isxdigit, '\t'));
    ASSERT(!do_ctype(isxdigit, EOF));
}

static void iscntrl_() {
    ASSERT(!do_ctype(iscntrl, 'a'));
    ASSERT(!do_ctype(iscntrl, 'A'));
    ASSERT(!do_ctype(iscntrl, 'z'));
    ASSERT(!do_ctype(iscntrl, 'Z'));
    ASSERT(!do_ctype(iscntrl, 'x'));
    ASSERT(!do_ctype(iscntrl, 'X'));
    ASSERT(!do_ctype(iscntrl, '8'));
    ASSERT(!do_ctype(iscntrl, '0'));
    ASSERT(!do_ctype(iscntrl, '9'));
    ASSERT(!do_ctype(iscntrl, '('));
    ASSERT(!do_ctype(iscntrl, '{'));
    ASSERT(!do_ctype(iscntrl, ' '));
    ASSERT(do_ctype(iscntrl, '\t'));
    ASSERT(do_ctype(iscntrl, '\0'));
    ASSERT(do_ctype(iscntrl, 127));
    ASSERT(do_ctype(iscntrl, 31));
    ASSERT(!do_ctype(iscntrl, EOF));
}

static void isgraph_() {
    ASSERT(do_ctype(isgraph, 'a'));
    ASSERT(do_ctype(isgraph, 'A'));
    ASSERT(do_ctype(isgraph, 'z'));
    ASSERT(do_ctype(isgraph, 'Z'));
    ASSERT(do_ctype(isgraph, 'x'));
    ASSERT(do_ctype(isgraph, 'X'));
    ASSERT(do_ctype(isgraph, '8'));
    ASSERT(do_ctype(isgraph, '0'));
    ASSERT(do_ctype(isgraph, '9'));
    ASSERT(do_ctype(isgraph, '('));
    ASSERT(do_ctype(isgraph, '{'));
    ASSERT(!do_ctype(isgraph, ' '));
    ASSERT(!do_ctype(isgraph, '\t'));
    ASSERT(!do_ctype(isgraph, '\0'));
    ASSERT(!do_ctype(isgraph, 127));
    ASSERT(!do_ctype(isgraph, 31));
    ASSERT(!do_ctype(isgraph, EOF));
}

static void isspace_() {
    ASSERT(!do_ctype(isspace, 'a'));
    ASSERT(!do_ctype(isspace, 'A'));
    ASSERT(!do_ctype(isspace, 'z'));
    ASSERT(!do_ctype(isspace, 'Z'));
    ASSERT(!do_ctype(isspace, 'x'));
    ASSERT(!do_ctype(isspace, 'X'));
    ASSERT(!do_ctype(isspace, '8'));
    ASSERT(!do_ctype(isspace, '0'));
    ASSERT(!do_ctype(isspace, '9'));
    ASSERT(!do_ctype(isspace, '('));
    ASSERT(!do_ctype(isspace, '{'));
    ASSERT(do_ctype(isspace, ' '));
    ASSERT(do_ctype(isspace, '\t'));
    ASSERT(do_ctype(isspace, '\v'));
    ASSERT(do_ctype(isspace, '\n'));
    ASSERT(do_ctype(isspace, '\f'));
    ASSERT(do_ctype(isspace, '\n'));
    ASSERT(!do_ctype(isspace, '\0'));
    ASSERT(!do_ctype(isspace, 127));
    ASSERT(!do_ctype(isspace, 31));
    ASSERT(!do_ctype(isspace, EOF));
}

static void isblank_() {
    ASSERT(!do_ctype(isblank, 'a'));
    ASSERT(!do_ctype(isblank, 'A'));
    ASSERT(!do_ctype(isblank, 'z'));
    ASSERT(!do_ctype(isblank, 'Z'));
    ASSERT(!do_ctype(isblank, 'x'));
    ASSERT(!do_ctype(isblank, 'X'));
    ASSERT(!do_ctype(isblank, '8'));
    ASSERT(!do_ctype(isblank, '0'));
    ASSERT(!do_ctype(isblank, '9'));
    ASSERT(!do_ctype(isblank, '('));
    ASSERT(!do_ctype(isblank, '{'));
    ASSERT(do_ctype(isblank, ' '));
    ASSERT(do_ctype(isblank, '\t'));
    ASSERT(!do_ctype(isblank, '\v'));
    ASSERT(!do_ctype(isblank, '\n'));
    ASSERT(!do_ctype(isblank, '\f'));
    ASSERT(!do_ctype(isblank, '\n'));
    ASSERT(!do_ctype(isblank, '\0'));
    ASSERT(!do_ctype(isblank, 127));
    ASSERT(!do_ctype(isblank, 31));
    ASSERT(!do_ctype(isblank, EOF));
}

static void isprint_() {
    ASSERT(do_ctype(isprint, 'a'));
    ASSERT(do_ctype(isprint, 'A'));
    ASSERT(do_ctype(isprint, 'z'));
    ASSERT(do_ctype(isprint, 'Z'));
    ASSERT(do_ctype(isprint, 'x'));
    ASSERT(do_ctype(isprint, 'X'));
    ASSERT(do_ctype(isprint, '8'));
    ASSERT(do_ctype(isprint, '0'));
    ASSERT(do_ctype(isprint, '9'));
    ASSERT(do_ctype(isprint, '('));
    ASSERT(do_ctype(isprint, '{'));
    ASSERT(do_ctype(isprint, ' '));
    ASSERT(!do_ctype(isprint, '\t'));
    ASSERT(!do_ctype(isprint, '\0'));
    ASSERT(!do_ctype(isprint, 127));
    ASSERT(!do_ctype(isprint, 31));
    ASSERT(!do_ctype(isprint, EOF));
}

static void ispunct_() {
    ASSERT(!do_ctype(ispunct, 'a'));
    ASSERT(!do_ctype(ispunct, 'A'));
    ASSERT(!do_ctype(ispunct, 'z'));
    ASSERT(!do_ctype(ispunct, 'Z'));
    ASSERT(!do_ctype(ispunct, 'x'));
    ASSERT(!do_ctype(ispunct, 'X'));
    ASSERT(!do_ctype(ispunct, '8'));
    ASSERT(!do_ctype(ispunct, '0'));
    ASSERT(!do_ctype(ispunct, '9'));
    ASSERT(do_ctype(ispunct, '('));
    ASSERT(do_ctype(ispunct, '{'));
    ASSERT(do_ctype(ispunct, '!'));
    ASSERT(do_ctype(ispunct, '/'));
    ASSERT(do_ctype(ispunct, ':'));
    ASSERT(do_ctype(ispunct, '@'));
    ASSERT(do_ctype(ispunct, '['));
    ASSERT(do_ctype(ispunct, '`'));
    ASSERT(do_ctype(ispunct, '~'));
    ASSERT(!do_ctype(ispunct, ' '));
    ASSERT(!do_ctype(ispunct, '\t'));
    ASSERT(!do_ctype(ispunct, '\0'));
    ASSERT(!do_ctype(ispunct, 127));
    ASSERT(!do_ctype(ispunct, 31));
    ASSERT(!do_ctype(ispunct, EOF));
}

static void tolower_() {
    ASSERT_EQ(do_toc(tolower, 'a'), 'a');
    ASSERT_EQ(do_toc(tolower, 'x'), 'x');
    ASSERT_EQ(do_toc(tolower, 'z'), 'z');
    ASSERT_EQ(do_toc(tolower, 'A'), 'a');
    ASSERT_EQ(do_toc(tolower, 'X'), 'x');
    ASSERT_EQ(do_toc(tolower, 'Z'), 'z');
    ASSERT_EQ(do_toc(tolower, '{'), '{');
    ASSERT_EQ(do_toc(tolower, ' '), ' ');
    ASSERT_EQ(do_toc(tolower, EOF), EOF);
}

static void toupper_() {
    ASSERT_EQ(do_toc(toupper, 'a'), 'A');
    ASSERT_EQ(do_toc(toupper, 'x'), 'X');
    ASSERT_EQ(do_toc(toupper, 'z'), 'Z');
    ASSERT_EQ(do_toc(toupper, 'A'), 'A');
    ASSERT_EQ(do_toc(toupper, 'X'), 'X');
    ASSERT_EQ(do_toc(toupper, 'Z'), 'Z');
    ASSERT_EQ(do_toc(toupper, '{'), '{');
    ASSERT_EQ(do_toc(toupper, ' '), ' ');
    ASSERT_EQ(do_toc(toupper, EOF), EOF);
}

TEST(ctype_h, isalnum_)
TEST(ctype_h, isalpha_)
TEST(ctype_h, isascii_)
TEST(ctype_h, islower_)
TEST(ctype_h, isupper_)
TEST(ctype_h, isdigit_)
TEST(ctype_h, isxdigit_)
TEST(ctype_h, iscntrl_)
TEST(ctype_h, isgraph_)
TEST(ctype_h, isspace_)
TEST(ctype_h, isblank_)
TEST(ctype_h, isprint_)
TEST(ctype_h, ispunct_)
TEST(ctype_h, tolower_)
TEST(ctype_h, toupper_)
}
