#include <ctype.h>

int isalpha(int c) {
    return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') ? 1 : 0;
}

int isblank(int c) {
    return (c == ' ' || c == '\t') ? 1 : 0;
}

int isspace(int c) {
    return (c == ' ' || c == '\n' || c == '\t' || c == '\v' || c == '\f' || c == '\r') ? 1 : 0;
}

int isdigit(int c) {
    return ('0'<= c && c <= '9') ? 1 : 0;
}