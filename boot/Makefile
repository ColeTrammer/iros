# Default host is given by script
DEFAULT_HOST!=../default-host.sh
# If it's not already defined, HOST = DEFAULT_HOST
HOST?=$(DEFAULT_HOST)
# Host achitecture is given by script
HOSTARCH!=../target-triplet-to-arch.sh $(HOST)

# Init flags if not already set (can be overridden)
CFLAGS?=-O2 -g
CPPFLAGS?=
LDFLAGS?=-nostdlib

# Include options that should always be set
CFLAGS:=$(CFLAGS) -ffreestanding -Wall -Wextra -std=gnu11
CPPFLAGS:=$(CPPFLAGS) -D__is_kernel -Iinclude
LDFLAFS:=$(LDFLAGS)

# Sets directories for install if not already set
DESTDIR?=
PREFIX?=
EXEC_PREFIX?=$(PREFIX)
BOOTDIR?=$(EXEC_PREFIX)/boot

# Include architecture specific options
ARCHDIR=arch/$(HOSTARCH)
include $(ARCHDIR)/make.config

# Sets default link script location if not set by arch's make config
LINK_SCRIPT?=$(ARCHDIR)/link.ld

# Updates flags
CFLAGS:=$(CFLAGS) $(KERNEL_ARCH_CFLAGS)
CPPFLAGS:=$(CPPFLAGS) $(KERNEL_ARCH_CPPFLAGS)
LDFLAGS:=$(LDFLAGS) $(KERNEL_ARCH_LDFLAGS)
LIBS:=$(LIBS) $(KERNEL_ARCH_LIBS)

# Lists kernel objects
KERNEL_OBJS=\
$(KERNEL_ARCH_OBJS)

# Lists all objects - includes extra objects needed for standard c programs
OBJS=\
$(KERNEL_OBJS)

.PHONY: all
all: boot_loader.o

# Makes kernel from link script with objects to link
boot_loader.o: $(OBJS) $(LINK_SCRIPT)
	$(CC) -T $(LINK_SCRIPT) -o $@ $(CFLAGS) $(LDFLAGS) $(OBJS)

# Builds any .o file from its .S counterpart

%.o: %.S
	$(CC) -o $@ -c $< $(CFLAGS) $(CPPFLAGS)

# Cleans by deleting .o files
.PHONY: clean
clean:
	rm -f boot_loader.o
	rm -f $(OBJS)

.PHONY: install install-boot-loader

install: install-boot-loader

install-boot-loader: boot_loader.o
	mkdir -p $(DESTDIR)$(BOOTDIR)
	cp --preserve=timestamps $< $(DESTDIR)$(BOOTDIR)