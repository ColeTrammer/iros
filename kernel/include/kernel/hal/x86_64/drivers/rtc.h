#ifndef _KERNEL_HAL_X86_64_DRIVERS_RTC_H
#define _KERNEL_HAL_X86_64_DRIVERS_RTC_H 1

#include <stdint.h>

#include <kernel/arch/x86_64/asm_utils.h>

#define RTC_IRQ_LINE 8

#define RTC_REGISTER_SELECT 0x70
#define RTC_DATA            0x71

#define RTC_SECONDS      0x00
#define RTC_MINUTES      0x02
#define RTC_HOURS        0x04
#define RTC_WEEKDAY      0x06
#define RTC_DAY_OF_MONTH 0x07
#define RTC_MONTH        0x08
#define RTC_YEAR         0x09
#define RTC_CENTURY      0x32

#define RTC_STATUS_A 0x0A
#define RTC_STATUS_B 0x0B
#define RTC_STATUS_C 0x0C

#define RTC_UPDATE_IN_PROGRESS 0x80

#define RTC_NOT_BCD     0x04
#define RTC_NOT_12_HOUR 0x02

#define RTC_DISABLE_NMI 0x80

#define RTC_BASE_RATE 32768

static inline uint8_t rtc_get(uint8_t reg) {
    outb(RTC_REGISTER_SELECT, RTC_DISABLE_NMI | (reg & 0x7F));
    return inb(RTC_DATA);
}

static inline void rtc_set(uint8_t reg, uint8_t val) {
    outb(RTC_REGISTER_SELECT, RTC_DISABLE_NMI | (reg & 0x7F));
    outb(RTC_DATA, val);
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

void init_RTC();

#endif /* _KERNEL_HAL_X86_64_DRIVERS_RTC_H */
