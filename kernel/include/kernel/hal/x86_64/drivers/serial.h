#ifndef _KERNEL_HAL_X86_64_DRIVERS_SERIAL_H
#define _KERNEL_HAL_X86_64_DRIVERS_SERIAL_H 1

#include <stdbool.h>
#include <sys/types.h>

#define SERIAL_13_IRQ_LINE 4
#define SERIAL_24_IRQ_LINE 3

#define SERIAL_COM1_PORT 0x3F8
#define SERIAL_COM2_PORT 0x2F8
#define SERIAL_COM3_PORT 0x3E8
#define SERIAL_COM4_PORT 0x2E8

#define SERIAL_DATA_OFFSET          0
#define SERIAL_INTERRUPT_OFFSET     1
#define SERIAL_BAUD_LOW_OFFSET      0
#define SERIAL_BAUD_HIGH_OFFSET     1
#define SERIAL_FIFO_OFFSET          2
#define SERIAL_CONTROL_OFFSET       3
#define SERIAL_MODEM_CONTROL_OFFSET 4
#define SERIAL_STATUS_OFFSET        5
#define SERIAL_MODEM_STATUS         6
#define SERIAL_SCRATCH              7

#define SERIAL_PORT(com_port, offset) ((com_port) + (offset))

#define SERIAL_BAUD_DIVISOR 3

#define SERIAL_5_BITS 0b00
#define sERIAL_6_BITS 0b01
#define SERIAL_7_BITS 0b10
#define SERIAL_8_BITS 0b11

#define SERIAL_STOP_1_BIT  0b000
#define SERIAL_STOP_2_BITS 0b100

#define SERIAL_PARITY_NONE  (0b000 << 3)
#define SERIAL_PARITY_ODD   (0b001 << 3)
#define SERIAL_PARITY_EVEN  (0b011 << 3)
#define SERIAL_PARITY_MARK  (0b101 << 3)
#define SERIAL_PARITY_SPACE (0b111 << 3)

#define SERIAL_DLAB 0x80

#define SERIAL_INTERRUPT_DATA   (1 << 0)
#define SERIAL_INTERRUPT_EMPY   (1 << 1)
#define SERIAL_INTERRUPT_ERROR  (1 << 2)
#define SERIAL_INTERRUPT_STATUS (1 << 3)

#define SERIAL_DATA_READY               (1 << 0)
#define SERIAL_OVERRUN_ERROR            (1 << 1)
#define SERIAL_PARITY_ERROR             (1 << 2)
#define SERIAL_FRAMING_ERROR            (1 << 3)
#define SERIAL_BREAK_INDICATOR          (1 << 4)
#define SERIAL_TRANSMITTER_EMPTY_BUFFER (1 << 5)
#define SERIAL_TRANSMITTER_EMPTY        (1 << 6)
#define SERIAL_IMPENDING_ERROR          (1 << 7)

void init_serial_ports();
void init_serial_port_device(dev_t dev);

bool serial_write_message(const char *s, size_t n);

#endif /* _KERNEL_HAL_X86_64_DRIVERS_SERIAL_H */