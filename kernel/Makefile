# Default host is given by script
DEFAULT_HOST!=../default-host.sh
# If it's not already defined, HOST = DEFAULT_HOST
HOST?=$(DEFAULT_HOST)
# Host achitecture is given by script
HOSTARCH!=../target-triplet-to-arch.sh $(HOST)

# Init flags if not already set (can be overridden)
CFLAGS=-O2
CPPFLAGS?=
LDFLAGS?=
LIBS?=

# Include options that should always be set
CFLAGS:=$(CFLAGS) -ffreestanding -Werror -Wall -Wextra -std=gnu2x
CPPFLAGS:=$(CPPFLAGS) -D__is_kernel -D_OS_2_SOURCE -Iinclude
LDFLAFS:=$(LDFLAGS)
LIBS:=$(LIBS) -nostdlib -lk -lgcc # links against libk and libgcc

# Sets directories for install if not already set
DESTDIR?=
PREFIX?=
EXEC_PREFIX?=$(PREFIX)
BOOTDIR?=$(EXEC_PREFIX)/boot
INCLUDEDIR?=$(PREFIX)/usr/include

# Include architecture specific options
ARCHDIR=arch/$(HOSTARCH)
HALDIR=hal/$(HOSTARCH)
include $(ARCHDIR)/make.config

# Sets default link script location if not set by arch's make config
LINK_SCRIPT?=$(ARCHDIR)/link.ld

# Updates flags
CFLAGS:=$(CFLAGS) $(KERNEL_ARCH_CFLAGS)
CPPFLAGS:=$(CPPFLAGS) $(KERNEL_ARCH_CPPFLAGS) -DARCH=$(HOSTARCH)
LDFLAGS:=$(LDFLAGS) $(KERNEL_ARCH_LDFLAGS)
LIBS:=$(LIBS) $(KERNEL_ARCH_LIBS)

# Lists kernel objects
KERNEL_OBJS=\
$(KERNEL_ARCH_OBJS) \
$(BUILDDIR)/kernel/fs/cached_dirent.o \
$(BUILDDIR)/kernel/fs/dev.o \
$(BUILDDIR)/kernel/fs/ext2.o \
$(BUILDDIR)/kernel/fs/initrd.o \
$(BUILDDIR)/kernel/fs/inode_store.o \
$(BUILDDIR)/kernel/fs/pipe.o \
$(BUILDDIR)/kernel/fs/procfs.o \
$(BUILDDIR)/kernel/fs/tmp.o \
$(BUILDDIR)/kernel/fs/tnode.o \
$(BUILDDIR)/kernel/fs/vfs.o \
$(BUILDDIR)/kernel/hal/devices.o \
$(BUILDDIR)/kernel/hal/ptmx.o \
$(BUILDDIR)/kernel/main/kernel.o \
$(BUILDDIR)/kernel/mem/anon_vm_object.o \
$(BUILDDIR)/kernel/mem/inode_vm_object.o \
$(BUILDDIR)/kernel/mem/page_frame_allocator.o \
$(BUILDDIR)/kernel/mem/phys_vm_object.o \
$(BUILDDIR)/kernel/mem/vm_allocator.o \
$(BUILDDIR)/kernel/mem/vm_object.o \
$(BUILDDIR)/kernel/mem/vm_region.o \
$(BUILDDIR)/kernel/net/arp.o \
$(BUILDDIR)/kernel/net/ethernet.o \
$(BUILDDIR)/kernel/net/icmp.o \
$(BUILDDIR)/kernel/net/inet_socket.o \
$(BUILDDIR)/kernel/net/interface.o \
$(BUILDDIR)/kernel/net/ip.o \
$(BUILDDIR)/kernel/net/loopback.o \
$(BUILDDIR)/kernel/net/mac.o \
$(BUILDDIR)/kernel/net/net.o \
$(BUILDDIR)/kernel/net/network_task.o \
$(BUILDDIR)/kernel/net/port.o \
$(BUILDDIR)/kernel/net/socket.o \
$(BUILDDIR)/kernel/net/tcp.o \
$(BUILDDIR)/kernel/net/udp.o \
$(BUILDDIR)/kernel/net/unix_socket.o \
$(BUILDDIR)/kernel/proc/blockers.o \
$(BUILDDIR)/kernel/proc/elf64.o \
$(BUILDDIR)/kernel/proc/pid.o \
$(BUILDDIR)/kernel/proc/process_state.o \
$(BUILDDIR)/kernel/proc/process.o \
$(BUILDDIR)/kernel/proc/task.o \
$(BUILDDIR)/kernel/proc/user_mutex.o \
$(BUILDDIR)/kernel/sched/task_sched.o \
$(BUILDDIR)/kernel/time/clock.o \
$(BUILDDIR)/kernel/time/timer.o \
$(BUILDDIR)/kernel/util/spinlock.o \
$(BUILDDIR)/kernel/util/hash_map.o \
$(BUILDDIR)/kernel/util/validators.o

# Lists all objects - includes extra objects needed for standard c programs
OBJS=\
$(BUILDDIR)/kernel/$(ARCHDIR)/crti.o \
$(BUILDDIR)/kernel/$(ARCHDIR)/crtbegin.o \
$(BUILDDIR)/kernel/$(ARCHDIR)/crt0.o \
$(KERNEL_OBJS) \
$(BUILDDIR)/kernel/$(ARCHDIR)/crtend.o \
$(BUILDDIR)/kernel/$(ARCHDIR)/crtn.o

# Lists all objects and libs in proper order
LINK_LIST=\
$(BUILDDIR)/kernel/$(ARCHDIR)/crti.o \
$(BUILDDIR)/kernel/$(ARCHDIR)/crtbegin.o \
$(BUILDDIR)/kernel/$(ARCHDIR)/crt0.o \
$(KERNEL_OBJS) \
$(LIBS) \
$(BUILDDIR)/kernel/$(ARCHDIR)/crtend.o \
$(BUILDDIR)/kernel/$(ARCHDIR)/crtn.o

.PHONY: all
all: $(BUILDDIR)/kernel/os_2.o

# Makes kernel from link script with objects to link
$(BUILDDIR)/kernel/os_2.o: $(OBJS) $(LINK_SCRIPT)
	$(CC) -T $(LINK_SCRIPT) -o $@ $(CFLAGS) $(LDFLAGS) $(LINK_LIST)

# Gets gcc's ctrbegin and crtend by asking for its path and copying it to where we want
$(BUILDDIR)/kernel/$(ARCHDIR)/crtbegin.o $(BUILDDIR)/kernel/$(ARCHDIR)/crtend.o:
	cp --preserve=timestamps `$(CC) $(CFLAGS) $(LDFLAGS) -print-file-name=$(@F)` $@

# Builds any .o file from its .c or .S, and generates its makefile dependencies with -MD
$(BUILDDIR)/%.o: $(ROOT)/%.c
	$(CC) -MD -o $@ -c $< $(CFLAGS) $(CPPFLAGS)

$(BUILDDIR)%.o: $(ROOT)/%.S
	$(CC) -o $@ -c $< $(CFLAGS) $(CPPFLAGS)

# Cleans by deleting .o, and .d files
.PHONY: clean
clean:
	rm -f os_2.o
	rm -f $(OBJS)
	rm -f $(OBJS:.o=.d) 

.PHONY: install install-headers install-kernel

install: install-headers install-kernel

# Installs headers by copying recursively
install-headers:
	mkdir -p $(DESTDIR)$(INCLUDEDIR)
	cp -R --preserve=timestamps -u include/. $(DESTDIR)$(INCLUDEDIR)/.

# Installs kernel via cp
install-kernel: $(BUILDDIR)/kernel/os_2.o
	mkdir -p $(DESTDIR)$(BOOTDIR)
	cp --preserve=timestamps $< $(DESTDIR)$(BOOTDIR)

# Includes the .d files for each object, if it exists.
# This will cause any object that depends on a specific header,
# that was changed, to be recompiled, as this include will 
# tell make its dependencies, which if altered, will cause
# make to go back and rebuild the object.
-include $(OBJS:.o=.d)