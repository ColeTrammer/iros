# Default host is given by script
DEFAULT_HOST!=../default-host.sh
# If it's not already defined, HOST = DEFAULT_HOST
HOST?=$(DEFAULT_HOST)
# Host achitecture is given by script
HOSTARCH!=../target-triplet-to-arch.sh $(HOST)

# Init flags if not already set (can be overridden)
CFLAGS?=-O2 -g
CPPFLAGS?=
LDFLAGS?=
LIBS?=

# Include options that should always be set
CFLAGS:=$(CFLAGS) -ffreestanding -Wall -Wextra -std=gnu11
CPPFLAGS:=$(CPPFLAGS) -D__is_boot_loader -Iinclude
LDFLAFS:=$(LDFLAGS)
LIBS:=$(LIBS) -nostdlib -lgcc # links against libgcc

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
CFLAGS:=$(CFLAGS) $(BOOT_LOADER_ARCH_CFLAGS)
CPPFLAGS:=$(CPPFLAGS) $(BOOT_LOADER_ARCH_CPPFLAGS)
LDFLAGS:=$(LDFLAGS) $(BOOT_LOADER_ARCH_LDFLAGS)
LIBS:=$(LIBS) $(BOOT_LOADER_ARCH_LIBS)

# Lists objects in boot loader
BOOT_LOADER_OBJS=\
$(BOOT_LOADER_ARCH_OBJS) \

# Lists all objects - includes extra objects needed for standard c programs
OBJS=\
$(BUILDDIR)/boot/$(ARCHDIR)/crti.o \
$(BUILDDIR)/boot/$(ARCHDIR)/crtbegin.o \
$(BOOT_LOADER_OBJS) \
$(BUILDDIR)/boot/$(ARCHDIR)/crtend.o \
$(BUILDDIR)/boot/$(ARCHDIR)/crtn.o

# Lists all objects and libs in proper order
LINK_LIST=\
$(BUILDDIR)/boot/$(ARCHDIR)/crti.o \
$(BUILDDIR)/boot/$(ARCHDIR)/crtbegin.o \
$(BOOT_LOADER_OBJS) \
$(LIBS) \
$(BUILDDIR)/boot/$(ARCHDIR)/crtend.o \
$(BUILDDIR)/boot/$(ARCHDIR)/crtn.o

.PHONY: all
all: $(BUILDDIR)/boot/boot_loader.o

# Makes kernel from link script with objects to link
$(BUILDDIR)/boot/boot_loader.o: $(OBJS) $(LINK_SCRIPT)
	$(CC) -T $(LINK_SCRIPT) -o $@ $(CFLAGS) $(LDFLAGS) $(LINK_LIST)

# Gets gcc's ctrbegin and crtend by asking for its path and copying it to where we want
$(BUILDDIR)/boot/$(ARCHDIR)/crtbegin.o $(BUILDDIR)/boot/$(ARCHDIR)/crtend.o:
	cp `$(CC) $(CFLAGS) $(LDFLAGS) -print-file-name=$(@F)` $@

# Builds any .o file from its .c or .S counterpart
%.o: %.c
	$(CC) -MD -o $@ -c $< $(CFLAGS) $(CPPFLAGS)

%.o: %.S
	$(CC) -o $@ -c $< $(CFLAGS) $(CPPFLAGS)

# Cleans by deleting .o files
.PHONY: clean
clean:
	rm -f boot_loader.o
	rm -f $(OBJS)
	rm -f $(OBJS:.o=.d)

.PHONY: install install-boot-loader

install: install-boot-loader

install-boot-loader: $(BUILDDIR)/boot/boot_loader.o
	mkdir -p $(DESTDIR)$(BOOTDIR)
	cp --preserve=timestamps $< $(DESTDIR)$(BOOTDIR)

# Includes the .d files for each object, if it exists.
# This will cause any object that depends on a specific header,
# that was changed, to be recompiled, as this include will 
# tell make its dependencies, which if altered, will cause
# make to go back and rebuild the object.
-include $(OBJS:.o=.d)