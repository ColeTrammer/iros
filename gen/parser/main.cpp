#include <assert.h>
#include <fcntl.h>
#include <liim/string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "lexer.h"

void print_usage_and_exit(char** argv) {
    fprintf(stderr, "Usage: %s <grammar>\n", argv[0]);
    exit(1);
}

int main(int argc, char** argv) {
    if (argc != 2) {
        print_usage_and_exit(argv);
    }

    struct stat info;
    if (stat(argv[1], &info) != 0) {
        perror("stat");
        return 1;
    }

    int fd = open(argv[1], O_RDONLY);
    char* contents = reinterpret_cast<char*>(mmap(nullptr, info.st_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0));
    if (contents == MAP_FAILED) {
        perror("mmap");
        return 1;
    }

    if (close(fd) != 0) {
        perror("close");
        return 1;
    }

    Lexer lexer(contents, info.st_size);
    auto tokens = lexer.lex();

    for (int i = 0; i < tokens.size(); i++) {
        auto& token = tokens[i];
        switch (token.type) {
#undef __ENUMERATE_TOKEN_TYPES
#define __ENUMERATE_TOKEN_TYPES(t)                                    \
    case Token::Type::Token##t:                                       \
        printf("Token: %s, '%s'\n", #t, String(token.text).string()); \
        break;
            ENUMERATE_TOKEN_TYPES
            default:
                assert(false);
        }
    }

    if (munmap(contents, info.st_size) != 0) {
        perror("munmap");
        return 1;
    }

    return 0;
}