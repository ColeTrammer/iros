#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>

#include <kernel/hal/output.h>

#include <kernel/hal/x86_64/drivers/vga.h>
#include <kernel/hal/x86_64/drivers/serial.h>

static size_t row = 0;
static size_t col = 0;

static size_t max_height = VGA_HEIGHT;
static size_t max_width = VGA_WIDTH;

static enum output_method method = OUTPUT_SCREEN;

static bool screen_print(const char *str, size_t len) {
    if (row >= max_height) {
        row = 0;
    }
    for (size_t i = 0; str[i] != '\0' && i < len; i++) {
        if (str[i] == '\n' || col >= max_width) {
            while (col < max_width) {
                write_vga_buffer(row, col++, ' ');
            }
            row++;
            col = 0;
            if (row >= max_height) {
                for (size_t r = 0; r < max_height - 1; r++) {
                    for (size_t c = 0; c < max_width; c++) {
                        write_vga_buffer(r, c, get_vga_buffer(r + 1, c));
                    }
                }
                for (size_t i = 0; i < max_width; i++) {
                    write_vga_buffer(max_height - 1, i, ' ');
                }
                row--;
            }
        } else {
            write_vga_buffer(row, col++, str[i]);
        }
    }
    return true;
}

static bool serial_print(const char *str, size_t len) {
    return serial_write_message(str, len);
}

void init_output() {
    for (size_t row = 0; row < max_height; row++) {
        for (size_t col = 0; col < max_width; col++) {
            write_vga_buffer(row, col, ' ');
        }
    }
}

bool kprint(const char *str, size_t len) {
    switch (method) {
        case OUTPUT_SCREEN: return screen_print(str, len);
        case OUTPUT_SERIAL: return serial_print(str, len);
        default:            return false;
    }
}

int debug_log(const char *format, ...) {
    va_list parameters;
    va_start(parameters, format);

    method = OUTPUT_SERIAL;
    int written = vprintf(format, parameters);
    method = OUTPUT_SCREEN;

    va_end(parameters);
    return written;
}

void dump_registers_to_screen() {
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
    
    set_vga_foreground(VGA_COLOR_RED);

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