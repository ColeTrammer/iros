#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "elf64.h"

#define MODULES_TAG_TYPE 3

#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_BASE ((uint16_t*) 0xB8000)
#define VGA_INDEX(row, col) ((row) * VGA_WIDTH + (col))
#define VGA_ENTRY(c, fg, bg) \
    (((uint16_t) (c) & 0x00FF) | ((uint16_t) (fg) << 8 & 0x0F00) | ((uint16_t) (bg) << 12 & 0xF000))

void clear_screen() {
    for (size_t row = 0; row < VGA_HEIGHT; row++) {
        for (size_t col = 0; col < VGA_WIDTH; col++) {
            VGA_BASE[VGA_INDEX(row, col)] = VGA_ENTRY(' ', 15, 0);
        }
    }
}

static size_t row = 0;

void kprint(const char *str) {
    if (row >= VGA_HEIGHT) {
        row = 0;
    }
    for (size_t i = 0; str[i] != '\0'; i++) {
        VGA_BASE[VGA_INDEX(row, i)] = VGA_ENTRY(str[i], 7, 0);
    }
    row++;
}

bool strequals(const char *s1, const char *s2) {
    size_t i = 0;
    for (; s1[i] != '\0' && s2[i] != '\0'; i++) {
        if (s1[i] != s2[i]) {
            return false;
        }
    }
    return s1[i] == s2[i];
}

void *memcpy(void *dest, const void *src, size_t n) {
    unsigned char *buffer = (unsigned char*) dest;
    const unsigned char *source = (const unsigned char*) src;
    for (size_t i = 0; i < n; i++) {
        buffer[i] = source[i];
    }
    return buffer;
}

void *memset(void *s, int c, size_t n) {
    unsigned char *buffer = (unsigned char*) s;
    for (size_t i = 0; i < n; i++) {
        buffer[i] = (unsigned char) c;
    }
    return buffer;
}

void map_kernel_elf(uint64_t kernel_start) {
    Elf64_Ehdr *kernel_elf_header = (Elf64_Ehdr*) kernel_start;
    Elf64_Phdr *kernel_program_header = (Elf64_Phdr*) (kernel_start + kernel_elf_header->e_phoff);
    uint64_t base = kernel_start + kernel_program_header->p_offset;
    uint64_t end = base + kernel_program_header->p_filesz;
    uint64_t addr = kernel_program_header->p_vaddr;
    memcpy((void*) addr, (void*) base, (size_t) (end - base)); 
}

uint64_t find_entry(uint64_t kernel_start) {
    Elf64_Ehdr *kernel_elf_header = (Elf64_Ehdr*) kernel_start;
    return kernel_elf_header->e_entry;
}

uint64_t get_kernel_size(uint64_t kernel_start) {
    Elf64_Ehdr *kernel_elf_header = (Elf64_Ehdr*) kernel_start;
    Elf64_Shdr *section_headers = (Elf64_Shdr*) (kernel_start + kernel_elf_header->e_shoff);
    for (size_t i = 0; i < kernel_elf_header->e_shnum; i++) {
        if (section_headers[i].sh_addr) {
            if (section_headers[i].sh_type == 8) {
                memset((void*) section_headers[i].sh_addr, 0, section_headers[i].sh_size);
                return section_headers[i].sh_addr - 0xFFFFFF0000000000 + section_headers[i].sh_size;
            }
        }
    }
    return 0;
}

struct boot_info {
    uint64_t kernel_entry;
    uint64_t kernel_phys_start;
    uint64_t kernel_phys_end;
    uint32_t *multiboot_info;
} __attribute__((packed));

static struct boot_info info;

struct boot_info *prepare_kernel_for_jump(uint32_t *multiboot_info) {
    clear_screen();
    info.multiboot_info = multiboot_info;
    if (multiboot_info[0] >= 8 && multiboot_info[1] == 0) {
        kprint("Found multiboot information structure.");
        multiboot_info += 2;
        while (multiboot_info[0] != MODULES_TAG_TYPE) {
            multiboot_info = (uint32_t*) ((uint64_t) multiboot_info + multiboot_info[1]);
            if ((uint64_t) multiboot_info % 8 != 0) {
                multiboot_info = (uint32_t*) (((uint64_t) multiboot_info & ~0x7) + 8);
            }
        }
        if (strequals((char*) &multiboot_info[4], "kernel")) {
            kprint("Found kernel.");
            uint64_t mod_start = (uint64_t) multiboot_info[2];
            uint64_t mod_end = (uint64_t) multiboot_info[3];
            if (mod_end > 0x600000) {
                kprint("Module not mapped into memory.");
                while (1);
            }
            map_kernel_elf(mod_start);
            info.kernel_entry = find_entry(mod_start);
            info.kernel_phys_start = 0x400000;
            info.kernel_phys_end = info.kernel_phys_start + get_kernel_size(mod_start);
            return &info;
        }
    } else {
        kprint("Failed.");
    }
    while (1);
}