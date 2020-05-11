#ifndef _KERNEL_HAL_X86_64_DRIVERS_BGA_H
#define _KERNEL_HAL_X86_64_DRIVERS_BGA_H 1

#include <kernel/arch/x86_64/asm_utils.h>
#include <kernel/hal/x86_64/drivers/pci.h>

#define BGA_INDEX_ID          0
#define BGA_INDEX_X_RES       1
#define BGA_INDEX_Y_RES       2
#define BGA_INDEX_BPP         3
#define BGA_INDEX_ENABLE      4
#define BGA_INDEX_BANK        5
#define BGA_INDEX_VIRT_WIDTH  6
#define BGA_INDEX_VIRT_HEIGHT 7
#define BGA_INDEX_X_OFFSET    8
#define BGA_INDEX_Y_OFFSET    9

#define BGA_VBE_DISABLED 0
#define BGA_VBE_ENABLED  1
#define BGA_LFB_ENABLED  0x40

#define BGA_INDEX_PORT 0x01CE
#define BGA_DATA_PORT  0x01CF

#define BGA_BPP_32 0x20

static inline uint16_t bga_read(uint16_t index) {
    outw(BGA_INDEX_PORT, index);
    return inw(BGA_DATA_PORT);
}

static inline void bga_write(uint16_t index, uint16_t val) {
    outw(BGA_INDEX_PORT, index);
    outw(BGA_DATA_PORT, val);
}

void init_bga(struct pci_configuration *config);

#endif /* _KERNEL_HAL_X86_64_DRIVERS_BGA_H */
