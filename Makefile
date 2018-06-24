# Lists all projects
PROJECTS=kernel libc boot
# Lists all projects that need headers installed
HEADER_PROJECTS=kernel libc

# Root defaults to cwd; must be set properly if using make -C
ROOT?=$(CURDIR)
# Sets directories for output based on defaults, and exports if needed
SYSROOT=$(ROOT)/sysroot
ISODIR?=$(ROOT)/isodir
export DESTDIR?=$(SYSROOT)

# Default host is given by script
DEFAULT_HOST!=./default-host.sh
# If it's not already defined, HOST = DEFAULT_HOST
HOST?=$(DEFAULT_HOST)
# Host achitecture is given by script
HOSTARCH!=./target-triplet-to-arch.sh $(HOST)

# Sets CC, AR, and OBJCOPY to respect host and use SYSROOT
export CC:=$(HOST)-gcc --sysroot=$(SYSROOT) # -isystem=$(SYSROOT)/usr/include
export AR:=$(HOST)-ar
export OBJCOPY:=$(HOST)-objcopy

.PHONY: all
all: os_2.iso

# Makes iso - headers must be installed first, then each PROJECT
# Makes iso by creating a directory with kernel image and grub.cfg,
# then calling grub-mkrescue appropriately
os_2.iso: install-headers $(PROJECTS)
	mkdir -p $(ISODIR)/boot/grub
	mkdir -p $(ISODIR)/modules
	$(OBJCOPY) -S $(SYSROOT)/boot/boot_loader.o $(ISODIR)/boot/boot_loader.o
	$(OBJCOPY) -S $(SYSROOT)/boot/os_2.o $(ISODIR)/modules/os_2.o
	cp $(ROOT)/grub.cfg $(ISODIR)/boot/grub
	grub-file --is-x86-multiboot2 $(ISODIR)/boot/boot_loader.o
	grub-mkrescue -o $(ROOT)/os_2.iso $(ISODIR)

# Makes project by calling its Makefile
.PHONY: $(PROJECTS)
$(PROJECTS):
	$(MAKE) install -C $(ROOT)/$@

# Makes the kernel depend on libc
kernel: libc

# Cleans by removing all output directories and calling each project's clean
.PHONY: clean
clean:
	rm -rf $(DESTDIR)
	rm -rf $(ISODIR)
	rm -f $(ROOT)/kernel.dis
	rm -f $(ROOT)/boot_loader.dis
	rm -f $(ROOT)/debug.log
	rm -f $(ROOT)/os_2.iso
	for dir in $(PROJECTS); do \
	  $(MAKE) clean -C $(ROOT)/$$dir; \
	done

.PHONY: run
run: all
	$(ROOT)/qemu.sh

# Installs headers by calling each project's install-headers
.PHONY: install-headers
install-headers:
	for dir in $(HEADER_PROJECTS); do \
	  $(MAKE) install-headers -C $(ROOT)/$$dir; \
	done
