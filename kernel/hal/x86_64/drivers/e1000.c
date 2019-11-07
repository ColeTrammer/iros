#include <arpa/inet.h>
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <kernel/hal/output.h>
#include <kernel/hal/x86_64/drivers/e1000.h>
#include <kernel/hal/x86_64/drivers/pic.h>
#include <kernel/mem/page.h>
#include <kernel/mem/vm_allocator.h>
#include <kernel/net/interface.h>

static struct network_interface *interface = NULL;

static uint32_t read_command(struct e1000_data *data, uint16_t offset) {
    return *((volatile uint32_t*) (data->mem_io_phys_base + offset));
}

static void write_command(struct e1000_data *data, uint16_t offset, uint32_t value) {
    *((volatile uint32_t*) (data->mem_io_phys_base + offset)) = value;
}

static bool has_eeprom(struct e1000_data *data) {
    // For some reason we need to check 1000 times
    write_command(data, E1000_EEPROM_REG, 1);
    for (int i = 0; i < 1000; i++) {
        if (read_command(data, E1000_EEPROM_REG) & 0x10) {
            return true;
        }
    }

    return false;
}

static void init_recieve_descriptors(struct e1000_data *data) {
    data->rx_descs_unaligned = calloc(E1000_NUM_RECIEVE_DESCS, sizeof(struct e1000_recieve_desc) + E1000_DESC_MIN_ALIGN);
    data->rx_descs = (struct e1000_recieve_desc*) (data->rx_descs_unaligned + E1000_DESC_MIN_ALIGN - (((uintptr_t) data->rx_descs_unaligned) % E1000_DESC_MIN_ALIGN));
    assert(((uintptr_t) data->rx_descs) % E1000_DESC_MIN_ALIGN == 0);

    for (int i = 0; i < E1000_NUM_RECIEVE_DESCS; i++) {
        data->rx_virt_addrs[i] = malloc(8192 + 16);
        data->rx_descs[i].addr = get_phys_addr((uintptr_t) data->rx_virt_addrs[i]);
        data->rx_descs[i].status = 0;
    }

    write_command(data, E1000_RX_DESC_HI, (uint32_t) (get_phys_addr((uintptr_t) data->rx_descs) >> 32));
    write_command(data, E1000_RX_DESC_LO, (uint32_t) (get_phys_addr((uintptr_t) data->rx_descs) & 0xFFFFFFFF));

    write_command(data, E1000_RX_DESC_LEN, E1000_NUM_RECIEVE_DESCS * sizeof(struct e1000_recieve_desc));
    write_command(data, E1000_RX_DESC_HEAD, 0);
    write_command(data, E1000_RX_DESC_TAIL, E1000_NUM_RECIEVE_DESCS - 1);

    write_command(data, E1000_RCTRL, E1000_RCTL_EN | E1000_RCTL_SBP | E1000_RCTL_UPE | E1000_RCTL_MPE | E1000_RCTL_LBM_NONE | E1000_RTCL_RDMTS_HALF | E1000_RCTL_BAM | E1000_RCTL_SECRC | E1000_RCTL_BSIZE_8192);
}

static void init_transmit_descriptors(struct e1000_data *data) {
    data->tx_descs_unaligned = calloc(E1000_NUM_TRANSMIT_DESCS, sizeof(struct e1000_transmit_desc) + E1000_DESC_MIN_ALIGN);
    data->tx_descs = (struct e1000_transmit_desc*) (data->tx_descs_unaligned + E1000_DESC_MIN_ALIGN - (((uintptr_t) data->tx_descs_unaligned) % E1000_DESC_MIN_ALIGN));
    assert(((uintptr_t) data->tx_descs) % E1000_DESC_MIN_ALIGN == 0);

    for (int i = 0; i < E1000_NUM_TRANSMIT_DESCS; i++) {
        data->tx_virt_addrs[i] = malloc(8192 + 16);
        data->tx_descs[i].addr = get_phys_addr((uintptr_t) data->tx_virt_addrs[i]);
        data->tx_descs[i].cmd = 0;
        data->tx_descs[i].status = E1000_TSTA_DD;
    }

    write_command(data, E1000_TX_DESC_HI, (uint32_t) (get_phys_addr((uintptr_t) data->tx_descs) >> 32));
    write_command(data, E1000_TX_DESC_LO, (uint32_t) (get_phys_addr((uintptr_t) data->tx_descs) & 0xFFFFFFFF));

    write_command(data, E1000_TX_DESC_LEN, E1000_NUM_RECIEVE_DESCS * sizeof(struct e1000_transmit_desc));
    write_command(data, E1000_TX_DESC_HEAD, 0);
    write_command(data, E1000_TX_DESC_TAIL, 0);

    write_command(data, E1000_TCTRL, 0b0110000000000111111000011111010U);
    write_command(data, E1000_TIPG_REG, 0x0060200AU);
}

static uint32_t read_eeprom(struct e1000_data *data, uint8_t addr) {
    write_command(data, E1000_EEPROM_REG, 1U | (((uint32_t) addr) << 8U));

    uint32_t val;
    while (!((val = read_command(data, E1000_EEPROM_REG)) & (1U << 4U)));

    return (uint16_t) ((val >> 16) & 0xFFFF);
}

static ssize_t e1000_send(struct network_interface *this, const void *raw, size_t len) {
    struct e1000_data *data = this->private_data;
    assert(len < 8192);

#ifdef KERNEL_E1000_DEBUG
    for (size_t i = 0; i < len; i++) {
        debug_log("TX Byte: [ %lu, %#2X ]\n", i, ((uint8_t*) raw)[i]);
    }
#endif /* KERNEL_E1000_DEBUG */

    memcpy(data->tx_virt_addrs[data->current_tx], raw, len);

    data->tx_descs[data->current_tx].length = len;
    data->tx_descs[data->current_tx].status = 0;
    data->tx_descs[data->current_tx].cmd = E1000_CMD_EOP | E1000_CMD_IFCS | E1000_CMD_RS;

    int save_current_tx = data->current_tx;
    data->current_tx = (data->current_tx + 1) % E1000_NUM_TRANSMIT_DESCS;

    write_command(data, E1000_TX_DESC_TAIL, data->current_tx);

    debug_log("Starting transmission...: [ %d ]\n", save_current_tx);

    while (!data->tx_descs[save_current_tx].status);

    debug_log("Finished transmitting...: [ %d ]\n", save_current_tx);
    return len;
}

static void e1000_recieve() {
    struct e1000_data *data = interface->private_data;

    while (data->rx_descs[data->current_rx].status & 0x1) {
        uint8_t *buf = data->rx_virt_addrs[data->current_rx];
        uint16_t len = data->rx_descs[data->current_rx].length;

        debug_log("Recieving packet...\n");

        interface->ops->recieve(interface, buf, len);

        data->rx_descs[data->current_rx].status = 0;

        int save_current_rx = data->current_rx;
        data->current_rx = (data->current_rx + 1) % E1000_NUM_RECIEVE_DESCS;
        write_command(data, E1000_RX_DESC_TAIL, save_current_rx);
    }
}

static void handle_interrupt() {
    struct e1000_data *data = interface->private_data;

    debug_log("Recived a interrupt for E1000\n");

    write_command(data, E1000_CTRL_IMASK, 1);
 
    uint32_t status = read_command(data, 0xc0);
    if (status & 0x04) {
        write_command(data, E1000_CTRL_REG, read_command(data, E1000_CTRL_REG) | E1000_ECTRL_SLU);
        debug_log("E1000 link up...\n");
    } else if (status & 0x10) {
        debug_log("Threshold ??\n");
        // Threshold ??
    } else if (status & 0x80) {
        e1000_recieve();
    }
}

static struct mac_address get_mac_address(struct network_interface *this) {
    struct mac_address addr;

    uint32_t val = read_eeprom(this->private_data, 0);
    addr.addr[0] = val & 0xFF;
    addr.addr[1] = val >> 8;
    val = read_eeprom(this->private_data, 1);
    addr.addr[2] = val & 0xFF;
    addr.addr[3] = val >> 8;
    val =read_eeprom(this->private_data, 2);
    addr.addr[4] = val & 0xFF;
    addr.addr[5] = val >> 8;

    return addr;
}

static struct network_interface_ops e1000_ops = {
    &e1000_send, NULL, &get_mac_address
};

void init_intel_e1000(struct pci_configuration *config) {
    debug_log("Found intel e1000 netword card: [ %u ]\n", config->interrupt_line);
    pci_enable_bus_mastering(config);

    assert(!(config->bar[0] & 1)); // Mem base
    assert(config->bar[1] & 1);    // Port base

    struct e1000_data *data = calloc(1, sizeof(struct e1000_data));
    data->mem_io_phys_base = (uintptr_t) create_phys_addr_mapping(config->bar[0]);
    data->io_port_base = config->bar[1] & ~1;

    debug_log("IO Bases: [ %#X, %#X ]\n", config->bar[0], data->io_port_base);

    assert(has_eeprom(data));
    debug_log("Has EEPROM: [ %s ]\n", "true");

    write_command(data, E1000_CTRL_REG, read_command(data, E1000_CTRL_REG) | E1000_ECTRL_SLU);

    init_recieve_descriptors(data);
    init_transmit_descriptors(data);

    write_command(data, E1000_CTRL_IMASK, 0x1F6DC);
    write_command(data, E1000_CTRL_IMASK, 0xFF & ~4);
    read_command(data, 0xC0);

    register_irq_line_handler(handle_interrupt, config->interrupt_line, true);

    interface = net_create_network_interface("e1000", NETWORK_INTERFACE_ETHERNET, &e1000_ops, data);
}