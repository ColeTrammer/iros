#ifndef _KERNEL_HAL_X86_64_DRIVERS_CMOS_H
#define _KERNEL_HAL_X86_64_DRIVERS_CMOS_H 1

#include <stdint.h>

#include <kernel/arch/x86_64/asm_utils.h>

#define CMOS_REGISTER_SELECT 0x70
#define CMOS_DATA            0x71

#define CMOS_SECONDS      0x00
#define CMOS_MINUTES      0x02
#define CMOS_HOURS        0x04
#define CMOS_WEEKDAY      0x06
#define CMOS_DAY_OF_MONTH 0x07
#define CMOS_MONTH        0x08
#define CMOS_YEAR         0x09
#define CMOS_CENTURY      0x32

#define CMOS_STATUS_A 0x0A
#define CMOS_STATUS_B 0x0B

#define CMOS_UPDATE_IN_PROGRESS 0x80

#define CMOS_NOT_BCD     0x04
#define CMOS_NOT_12_HOUR 0x02

#define CMOS_DISABLE_NMI 0

static inline uint8_t cmos_get(uint8_t reg) {
    outb(CMOS_REGISTER_SELECT, (CMOS_DISABLE_NMI << 7) | (reg & 0x7F));
    return inb(CMOS_DATA);
}

static inline void cmos_set(uint8_t reg, uint8_t val) {
    outb(CMOS_REGISTER_SELECT, (CMOS_DISABLE_NMI << 7) | (reg & 0x7F));
    outb(CMOS_DATA, val);
}

struct rtc_time {
    uint8_t second;
    uint8_t minute;
    uint8_t hour;
    uint8_t day;
    uint8_t month;
    uint8_t year;
    uint8_t century;
} __attribute__((packed));

void init_cmos();

#endif /* _KERNEL_HAL_X86_64_DRIVERS_CMOS_H */