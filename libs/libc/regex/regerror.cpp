#include <regex.h>
#include <string.h>

extern "C" size_t regerror(int error, const regex_t* __restrict, char* __restrict buf, size_t buffer_len) {
    const char* str;
    switch (error) {
        case REG_BADBR:
            str = "Invalid duplicate count";
            break;
        case REG_BADPAT:
            str = "Invalid pattern";
            break;
        case REG_BADRPT:
            str = "Invalid repetition";
            break;
        case REG_EBRACE:
            str = "Unmatched {} pair";
            break;
        case REG_EBRACK:
            str = "Unmatched [] pair";
            break;
        case REG_ECOLLATE:
            str = "Invalid collating element";
            break;
        case REG_ECTYPE:
            str = "Invalid character class";
            break;
        case REG_EESCAPE:
            str = "Invalid trailing backslash";
            break;
        case REG_EPAREN:
            str = "Unmatched () pair";
            break;
        case REG_ESPACE:
            str = "Ran out of memory";
            break;
        case REG_ESUBREG:
            str = "Invalid back reference to a subexpression";
            break;
        case REG_NOMATCH:
            str = "No matches";
            break;
        default:
            str = "Unknown error";
            break;
    }
    size_t len = strlen(str);
    if (buf && buffer_len > 0) {
        strncpy(buf, str, buffer_len - 1);
        buf[buffer_len - 1] = '\0';
    }
    return len + 1;
}
