#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <kernel/display/vga.h>
#include <kernel/display/terminal.h>

static size_t row = 0;
static size_t col = 0;
static enum vga_color foreground = VGA_COLOR_WHITE;
static enum vga_color background = VGA_COLOR_BLACK;

void clear_terminal() {
    for (size_t row = 0; row < VGA_HEIGHT; row++) {
        for (size_t col = 0; col < VGA_WIDTH; col++) {
            VGA_BASE[VGA_INDEX(row, col)] = VGA_ENTRY(' ', foreground, background);
        }
    }
}

bool kprint(const char *str, size_t len) {
    if (row >= VGA_HEIGHT) {
        row = 0;
    }
    for (size_t i = 0; str[i] != '\0' && i < len; i++) {
        if (str[i] == '\n' || col >= VGA_WIDTH) {
            row++;
            col = 0;
            if (row >= VGA_HEIGHT) {
                memmove(VGA_BASE, VGA_BASE + VGA_WIDTH, VGA_WIDTH * (VGA_HEIGHT - 1) * sizeof(uint16_t) / sizeof(unsigned char));
                row--;
            }
        } else {
            VGA_BASE[VGA_INDEX(row, col++)] = VGA_ENTRY(str[i], foreground, background);
        }
    }
    return true;
}

void dump_registers() {
    uint64_t rax, rbx, rcx, rdx, rbp, rsp, rsi, rdi, r8, r9, r10, r11, r12, r13, r14, r15;
    asm( "mov %%rax, %0 " : "=m"(rax) );
    asm( "mov %%rbx, %0 " : "=m"(rbx) );
    asm( "mov %%rcx, %0 " : "=m"(rcx) );
    asm( "mov %%rdx, %0 " : "=m"(rdx) );
    asm( "mov %%rbp, %0 " : "=m"(rbp) );
    asm( "mov %%rsp, %0 " : "=m"(rsp) );
    asm( "mov %%rsi, %0 " : "=m"(rsi) );
    asm( "mov %%rdi, %0 " : "=m"(rdi) );
    asm( "mov %%r8 , %0 " : "=m"(r8 ) );
    asm( "mov %%r9 , %0 " : "=m"(r9 ) );
    asm( "mov %%r10, %0 " : "=m"(r10) );
    asm( "mov %%r11, %0 " : "=m"(r11) );
    asm( "mov %%r12, %0 " : "=m"(r12) );
    asm( "mov %%r13, %0 " : "=m"(r13) );
    asm( "mov %%r14, %0 " : "=m"(r14) );
    asm( "mov %%r15, %0 " : "=m"(r15) );
    printf("RAX=%#.16lX RBX=%#.16lX\n", rax, rbx);
    printf("RCX=%#.16lX RDX=%#.16lX\n", rcx, rdx);
    printf("RBP=%#.16lX RSP=%#.16lX\n", rbp, rsp);
    printf("RSI=%#.16lX RDI=%#.16lX\n", rsi, rdi);
    printf("R8 =%#.16lX R9 =%#.16lX\n", r8 , r9 );
    printf("R10=%#.16lX R11=%#.16lX\n", r10, r11);
    printf("R12=%#.16lX R13=%#.16lX\n", r12, r13);
    printf("R14=%#.16lX R15=%#.16lX\n", r14, r15);
}

void set_foreground(enum vga_color _foreground) {
    foreground = _foreground;
}

void set_background(enum vga_color _background) {
    background = _background;
}