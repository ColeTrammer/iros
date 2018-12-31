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
CPPFLAGS:=$(CPPFLAGS) -D__is_kernel -Iinclude
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
$(KERNEL_ARCH_OBJS) \
display/terminal.o \
interrupts/interrupts.o \
main/kernel.o \
mem/page_frame_allocator.o

# Lists all objects - includes extra objects needed for standard c programs
OBJS=\
$(ARCHDIR)/crti.o \
$(ARCHDIR)/crtbegin.o \
$(ARCHDIR)/crt0.o \
$(KERNEL_OBJS) \
$(ARCHDIR)/crtend.o \
$(ARCHDIR)/crtn.o

# Lists all objects and libs in proper order
LINK_LIST=\
$(ARCHDIR)/crti.o \
$(ARCHDIR)/crtbegin.o \
$(ARCHDIR)/crt0.o \
$(KERNEL_OBJS) \
$(LIBS) \
$(ARCHDIR)/crtend.o \
$(ARCHDIR)/crtn.o

.PHONY: all
all: os_2.o

# Makes kernel from link script with objects to link
os_2.o: $(OBJS) $(LINK_SCRIPT)
	$(CC) -T $(LINK_SCRIPT) -o $@ $(CFLAGS) $(LDFLAGS) $(LINK_LIST)

# Gets gcc's ctrbegin and crtend by asking for its path and copying it to where we want
$(ARCHDIR)/crtbegin.o $(ARCHDIR)/crtend.o:
	cp `$(CC) $(CFLAGS) $(LDFLAGS) -print-file-name=$(@F)` $@

# Builds any .o file from its .c or .S, and generates its makefile dependencies with -MD
%.o: %.c
	$(CC) -MD -o $@ -c $< $(CFLAGS) $(CPPFLAGS)

%.o: %.S
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
	cp -R --preserve=timestamps include/. $(DESTDIR)$(INCLUDEDIR)/.

# Installs kernel via cp
install-kernel: os_2.o
	mkdir -p $(DESTDIR)$(BOOTDIR)
	cp --preserve=timestamps $< $(DESTDIR)$(BOOTDIR)

# Includes the .d files for each object, if it exists.
# This will cause any object that depends on a specific header,
# that was changed, to be recompiled, as this include will 
# tell make its dependencies, which if altered, will cause
# make to go back and rebuild the object.
-include $(OBJS:.o=.d)