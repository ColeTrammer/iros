# Lists all projects
PROJECTS=kernel libs boot gen initrd userland
# Lists all projects that need headers installed
HEADER_PROJECTS=gen kernel libs userland
# Lists all projects that need to be built on host
NATIVE_PROJECTS=gen

# Root defaults to cwd; must be set properly if using make -C
export ROOT?=$(CURDIR)
# Sets directories for output based on defaults, and exports if needed
export SYSROOT=$(ROOT)/sysroot
export ISODIR?=$(ROOT)/isodir
export BUILDDIR?=$(ROOT)/build
export DESTDIR?=$(SYSROOT)

# Default host is given by script
DEFAULT_HOST!=./default-host.sh
# If it's not already defined, HOST = DEFAULT_HOST
export HOST?=$(DEFAULT_HOST)
# Host achitecture is given by script
export HOSTARCH!=./target-triplet-to-arch.sh $(HOST)

# Sets CC, AR, and OBJCOPY to respect host and use SYSROOT
export CC:=$(HOST)-gcc --sysroot=$(SYSROOT) -isystem=$(SYSROOT)/usr/include $(DEFINES) -std=gnu2x
export CXX:=$(HOST)-g++ --sysroot=$(SYSROOT) -isystem=$(SYSROOT)/usr/include $(DEFINES) -std=c++2a -fconcepts -fno-exceptions -fno-rtti
export PARSER:=$(BUILDDIR)/gen/parser/parser.native
export CFLAGS:=-fno-omit-frame-pointer -fno-inline -g -O2
export LD:=$(CC)
export AR:=$(HOST)-ar
export OBJCOPY:=$(HOST)-objcopy

.PHONY: all
all: os_2.iso os_2.img

# Makes iso - headers must be installed first, then each PROJECT
# Makes iso by creating a directory with kernel image and grub.cfg,
# then calling grub-mkrescue appropriately
os_2.iso: prepare-build install-headers $(PROJECTS)
	mkdir -p $(ISODIR)/boot/grub
	mkdir -p $(ISODIR)/modules
	$(OBJCOPY) -S $(SYSROOT)/boot/boot_loader.o $(ISODIR)/boot/boot_loader.o
	$(OBJCOPY) -S $(SYSROOT)/boot/os_2.o $(ISODIR)/modules/os_2.o
	cp --preserve=timestamps $(SYSROOT)/boot/initrd.bin $(ISODIR)/modules/initrd.bin
	cp --preserve=timestamps $(ROOT)/grub.cfg $(ISODIR)/boot/grub
	grub-file --is-x86-multiboot2 $(ISODIR)/boot/boot_loader.o
	grub-mkrescue -o $@ $(ISODIR)

os_2.img: $(PROJECTS)
	sudo $(ROOT)/makeimg.sh

# Makes project by calling its Makefile
.PHONY: $(PROJECTS)
$(PROJECTS):
	$(MAKE) install -C $(ROOT)/$@

$(PROJECTS): native

# Makes the kernel depend on libs
kernel: libs

# Makes the initrd depend on libs
initrd: libs

# Makes the userland depend on libs
userland: libs

.PHONY: native
native:
	for dir in $(NATIVE_PROJECTS); do \
	  $(MAKE) native -C $(ROOT)/$$dir; \
	done

# Cleans by removing all output directories and calling each project's clean
.PHONY: clean
clean:
	rm -rf $(ISODIR)
	rm -rf $(BUILDDIR)
	rm -f $(ROOT)/initrd/files/*.o
	rm -f $(ROOT)/*.dis
	rm -f $(ROOT)/debug.log
	rm -f $(ROOT)/os_2.iso

.PHONY: run
run:
	$(ROOT)/qemu.sh

.PHONY: debug
debug:
	$(ROOT)/qemu.sh --debug

SOURCES+=$(shell find $(ROOT)/boot -type f \( -name '*.c' -o -name '*.S' -o -name '*.cpp' \))
SOURCES+=$(shell find $(ROOT)/gen -type f \( -name '*.c' -o -name '*.S' -o -name '*.cpp' \))
SOURCES+=$(shell find $(ROOT)/initrd -type f \( -name '*.c' -o -name '*.S' -o -name '*.cpp' \))
SOURCES+=$(shell find $(ROOT)/kernel -type f \( -name '*.c' -o -name '*.S' -o -name '*.cpp' \))
SOURCES+=$(shell find $(ROOT)/libs -type f \( -name '*.c' -o -name '*.S' -o -name '*.cpp' \))
SOURCES+=$(shell find $(ROOT)/userland -type f \( -name '*.c' -o -name '*.S' -o -name '*.cpp' \))
OBJECTS+=$(patsubst $(ROOT)/%.c, $(BUILDDIR)/%.o, $(SOURCES))
OBJECTS+=$(patsubst $(ROOT)/%.S, $(BUILDDIR)/%.o, $(SOURCES))
OBJECTS+=$(patsubst $(ROOT)/%.cpp, $(BUILDDIR)/%.o, $(SOURCES))
OBJDIRS:=$(dir $(OBJECTS))

.PHONY: prepare-build
prepare-build:
	mkdir --parents $(OBJDIRS)

# Installs headers by calling each project's install-headers
.PHONY: install-headers
install-headers:
	mkdir -p $(SYSROOT)/bin
	for dir in $(HEADER_PROJECTS); do \
	  $(MAKE) install-headers -C $(ROOT)/$$dir; \
	done

disassemble: all
	$(HOST)-objdump -D $(SYSROOT)/boot/boot_loader.o > $(ROOT)/boot_loader.dis
	$(HOST)-objdump -D $(SYSROOT)/boot/os_2.o > $(ROOT)/kernel.dis