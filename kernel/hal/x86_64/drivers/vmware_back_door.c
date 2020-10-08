#include <kernel/hal/hw_device.h>
#include <kernel/hal/input.h>
#include <kernel/hal/isa_driver.h>
#include <kernel/hal/output.h>
#include <kernel/hal/x86_64/drivers/vmware_back_door.h>
#include <kernel/util/init.h>

static bool is_enabled;
static bool is_vmmouse_enabled;

bool vmmouse_is_enabled(void) {
    return is_vmmouse_enabled;
}

static void enable_vmmouse(void) {
    vmware_send(VMWARE_VMMOUSE_COMMAND, VMMOUSE_ABSOLUTE_POSITION);
    is_vmmouse_enabled = true;
}

// This dodesn't actually disable the vmmouse, but does disable the lack
// of mouse grabbing in QEMU.
static void disable_vmmouse(void) {
    vmware_send(VMWARE_VMMOUSE_COMMAND, VMMOUSE_RELATIVE_POSITION);
    is_vmmouse_enabled = false;
}

static struct mouse_event s_event;
static bool left_is_down;
static bool right_is_down;

struct mouse_event *vmmouse_read(void) {
    uint32_t status = vmware_send(VMWARE_VMMOUSE_STATUS, 0);
    if (status == 0xFFFF0000) {
        // reset the device
        disable_vmmouse();
        enable_vmmouse();
        return NULL;
    }

    int count = status & 0xFFFF;
    // This syncs the vmmouse with the PS/2 mouse's IRQ mechanism, since
    // the PS/2 mouse expects 4 interrutps before a packet is fully complete.
    if (!count || count % 4 != 0) {
        return NULL;
    }

    struct vmware_registers regs = vmware_send_full(VMWARE_VMMOUSE_DATA, 4);
    s_event.dx = regs.ebx;
    s_event.dy = regs.ecx;

    if (regs.eax & 0x20) {
        if (left_is_down) {
            s_event.left = MOUSE_NO_CHANGE;
        } else {
            s_event.left = MOUSE_DOWN;
            left_is_down = true;
        }
    } else {
        if (!left_is_down) {
            s_event.left = MOUSE_NO_CHANGE;
        } else {
            s_event.left = MOUSE_UP;
            left_is_down = false;
        }
    }

    if (regs.eax & 0x10) {
        if (right_is_down) {
            s_event.right = MOUSE_NO_CHANGE;
        } else {
            s_event.right = MOUSE_DOWN;
            right_is_down = true;
        }
    } else {
        if (!right_is_down) {
            s_event.right = MOUSE_NO_CHANGE;
        } else {
            s_event.right = MOUSE_UP;
            right_is_down = false;
        }
    }

    s_event.scroll_state = regs.edx == 0xFFFFFFFF ? SCROLL_UP : regs.edx == 0x01 ? SCROLL_DOWN : SCROLL_NONE;
    s_event.scale_mode = SCALE_ABSOLUTE;
    return &s_event;
}

static void init_vmmouse(void) {
    vmware_send(VMWARE_VMMOUSE_COMMAND, VMMOUSE_GET_ID);
    uint32_t result = vmware_send(VMWARE_VMMOUSE_DATA, 1);
    if (result != VMMOUSE_QEMU_VERSION) {
        debug_log("vmmouse not detected: [ %#.8X ]\n", result);
        return;
    }

    debug_log("initializing vmmouse\n");
    enable_vmmouse();
}

bool vmware_back_door_is_enabled(void) {
    return is_enabled;
}

static void detect_vmware_back_door(struct hw_device *parent) {
    uint32_t result = vmware_send(VMWARE_GET_VERSION, ~VMWARE_MAGIC);
    if (result == 0xFFFFFFFF) {
        debug_log("vmware back door not detected\n");
        return;
    }

    debug_log("initializing vmware back door\n");
    struct hw_device *device = create_hw_device("VMWare Backdoor", parent, hw_device_id_isa(), NULL);
    device->status = HW_STATUS_ACTIVE;
    init_vmmouse();

    is_enabled = true;
}

static struct isa_driver vmware_back_door_driver = {
    .name = "VMWare Backdoor",
    .detect_devices = detect_vmware_back_door,
};

static void init_vmware_back_door(void) {
    register_isa_driver(&vmware_back_door_driver);
}
INIT_FUNCTION(init_vmware_back_door, driver);
