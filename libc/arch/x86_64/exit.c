#include <stdlib.h>

__attribute__((__noreturn__))
void exit(int status) {
    asm( "movq $1, %%rdi\n"\
         "movq %0, %%rsi\n"\
         "int $0x80" : : "m"(status) : "rdi", "rsi" );
    
    while (1);
    __builtin_unreachable();
}