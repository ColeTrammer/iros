#include <stdint.h>
#include <stdio.h>

#include <kernel/display/terminal.h>

void dump_registers() {
    uint64_t rax, rbx, rcx, rdx, rbp, rsp, rsi, rdi, r8, r9, r10, r11, r12, r13, r14, r15, cr3;
    asm( "mov %%rax, %0" : "=m"(rax) );
    asm( "mov %%rbx, %0" : "=m"(rbx) );
    asm( "mov %%rcx, %0" : "=m"(rcx) );
    asm( "mov %%rdx, %0" : "=m"(rdx) );
    asm( "mov %%rbp, %0" : "=m"(rbp) );
    asm( "mov %%rsp, %0" : "=m"(rsp) );
    asm( "mov %%rsi, %0" : "=m"(rsi) );
    asm( "mov %%rdi, %0" : "=m"(rdi) );
    asm( "mov %%r8 , %0" : "=m"(r8 ) );
    asm( "mov %%r9 , %0" : "=m"(r9 ) );
    asm( "mov %%r10, %0" : "=m"(r10) );
    asm( "mov %%r11, %0" : "=m"(r11) );
    asm( "mov %%r12, %0" : "=m"(r12) );
    asm( "mov %%r13, %0" : "=m"(r13) );
    asm( "mov %%r14, %0" : "=m"(r14) );
    asm( "mov %%r15, %0" : "=m"(r15) );
    asm( "mov %%cr3, %%rdx\n"\
         "mov %%rdx, %0" : "=m"(cr3) : : "rdx" );
    printf("RAX=%#.16lX RBX=%#.16lX\n", rax, rbx);
    printf("RCX=%#.16lX RDX=%#.16lX\n", rcx, rdx);
    printf("RBP=%#.16lX RSP=%#.16lX\n", rbp, rsp);
    printf("RSI=%#.16lX RDI=%#.16lX\n", rsi, rdi);
    printf("R8 =%#.16lX R9 =%#.16lX\n", r8 , r9 );
    printf("R10=%#.16lX R11=%#.16lX\n", r10, r11);
    printf("R12=%#.16lX R13=%#.16lX\n", r12, r13);
    printf("R14=%#.16lX R15=%#.16lX\n", r14, r15);
    printf("CR3=%#.16lX\n", cr3);
}