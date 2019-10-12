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
CFLAGS:=$(CFLAGS) -ffreestanding -Werror -Wall -Wextra -std=gnu11
CPPFLAGS:=$(CPPFLAGS) -D__is_libc -Iinclude
LIBK_CFLAGS:=$(CFLAGS)
LIBK_CPPFLAGS:=$(CPPFLAGS) -D__is_libk

# Sets directories for install if not already set
DESTDIR?=
PREFIX?=/usr
EXEC_PREFIX?=$(PREFIX)
INCLUDEDIR?=$(PREFIX)/include
LIBDIR?=$(EXEC_PREFIX)/lib

# Include architecture specific options
ARCHDIR=arch/$(HOSTARCH)
include $(ARCHDIR)/make.config

# Updates flags
CFLAGS:=$(CFLAGS) $(ARCH_CFLAGS)
CPPFLAGS:=$(CPPFLAGS) $(ARCH_CPPFLAGS) -DARCH=$(HOSTARCH)
LIBK_CFLAGS:=$(LIBK_CFLAGS) $(KERNEL_ARCH_CFLAGS)
LIBK_CPPFLAGS:=$(LIBK_CPPFLAGS) $(KERNEL_ARCH_CPPFLAGS) -DARCH=$(HOSTARCH)

# Lists objects used in 'free' enviornment
FREEOBJS=\
$(ARCH_FREEOBJS) \
$(BUILDDIR)/libc/ctype/ctype.o \
$(BUILDDIR)/libc/errno/errno.o \
$(BUILDDIR)/libc/stdio/printf.o \
$(BUILDDIR)/libc/stdio/scanf.o \
$(BUILDDIR)/libc/stdlib/abort.o \
$(BUILDDIR)/libc/stdlib/conversions.o \
$(BUILDDIR)/libc/stdlib/malloc.o \
$(BUILDDIR)/libc/stdlib/stdlib.o \
$(BUILDDIR)/libc/string/memchr.o \
$(BUILDDIR)/libc/string/memcmp.o \
$(BUILDDIR)/libc/string/memcpy.o \
$(BUILDDIR)/libc/string/memmove.o \
$(BUILDDIR)/libc/string/memset.o \
$(BUILDDIR)/libc/string/strcat.o \
$(BUILDDIR)/libc/string/strchr.o \
$(BUILDDIR)/libc/string/strcmp.o \
$(BUILDDIR)/libc/string/strcpy.o \
$(BUILDDIR)/libc/string/strcspn.o \
$(BUILDDIR)/libc/string/strdup.o \
$(BUILDDIR)/libc/string/strlen.o \
$(BUILDDIR)/libc/string/strcat.o \
$(BUILDDIR)/libc/string/strncmp.o \
$(BUILDDIR)/libc/string/strncpy.o \
$(BUILDDIR)/libc/string/strpbrk.o \
$(BUILDDIR)/libc/string/strrchr.o \
$(BUILDDIR)/libc/string/strspn.o \
$(BUILDDIR)/libc/string/strstr.o \
$(BUILDDIR)/libc/string/strtok.o

# Lists objects used in 'hosted' enviornment
HOSTEDOBJS=\
$(ARCH_HOSTEDOBJS) \
$(BUILDDIR)/libc/assert/assert.o \
$(BUILDDIR)/libc/dirent/dirent.o \
$(BUILDDIR)/libc/stdio/files.o \
$(BUILDDIR)/libc/stdio/puts.o \
$(BUILDDIR)/libc/stdlib/atexit.o \
$(BUILDDIR)/libc/stdlib/env.o \
$(BUILDDIR)/libc/string/strcoll.o \
$(BUILDDIR)/libc/string/strerror.o \
$(BUILDDIR)/libc/string/strxfrm.o \
$(BUILDDIR)/libc/termios/termios.o \
$(BUILDDIR)/libc/time/time.o \
$(BUILDDIR)/libc/unistd/exec.o \
$(BUILDDIR)/libc/unistd/getopt.o \
$(BUILDDIR)/libc/unistd/isatty.o \
$(BUILDDIR)/libc/init.o

# Lists all objects
OBJS=\
$(FREEOBJS) \
$(HOSTEDOBJS)

# Lists objects used by libk
LIBK_OBJS=$(FREEOBJS:.o=.libk.o)

# List objects used by libm
LIBM_OBJS=\
$(BUILDDIR)/libc/math/math.o

# List of binaries to be created
BINARIES=$(BUILDDIR)/libc/libc.a $(BUILDDIR)/libc/libk.a $(BUILDDIR)/libc/libm.a

.PHONY: all
all: $(BINARIES)

# Makes library file from objs using AR
$(BUILDDIR)/libc/libc.a: $(OBJS)
	$(AR) rcs $@ $^

$(BUILDDIR)/libc/libk.a: $(LIBK_OBJS)
	$(AR) rcs $@ $^

$(BUILDDIR)/libc/libm.a: $(LIBM_OBJS)
	$(AR) rcs $@ $^

# Makes object file from any .c or .S file
%.o: %.c
	$(CC) -MD -c $< -o $@ $(CFLAGS) $(CPPFLAGS)

%.o: %.S
	$(CC) -c $< -o $@ $(CFLAFS) $(CPPFLAGS)

%.libk.o: %.c
	$(CC) -MD -c $< -o $@ $(LIBK_CFLAGS) $(LIBK_CPPFLAGS)

%.libk.o: %.S
	$(CC) -c $< -o $@ $(LIBK_CFLAGS) $(LIBK_CPPFLAGS)

# Cleans by removing all .o, .d, and .a files
.PHONY: clean
clean:
	rm -f $(BINARIES)
	rm -f $(OBJS) $(LIBK_OBJS) $(ARCH_RAW_OBJS) $(LIBM_OBJS)
	rm -f $(OBJS:.o=.d) $(LIBK_OBJS:.o=.d) $(ARCH_RAW_OBJS:.o=.d) $(LIBM_OBJS:.o=.d)

.PHONY: install install-headers install-libs install-raw-objs
install: install-headers install-libs install-raw-objs

# Installs headers
install-headers:
	mkdir -p $(DESTDIR)$(INCLUDEDIR)
	cp -R --preserve=timestamps -u include/. $(DESTDIR)$(INCLUDEDIR)/.

# Installs libs
install-libs: $(BINARIES)
	mkdir -p $(DESTDIR)$(LIBDIR)
	cp --preserve=timestamps $^ $(DESTDIR)$(LIBDIR)

# Installs raw objs needed (i.e. crt0.o)
install-raw-objs: $(ARCH_RAW_OBJS)
	mkdir -p $(DESTDIR)$(LIBDIR)
	cp --preserve=timestamps $^ $(DESTDIR)$(LIBDIR)

# Includes the .d files for each object, if it exists.
# This will cause any object that depends on a specific header,
# that was changed, to be recompiled, as this include will 
# tell make its dependencies, which if altered, will cause
# make to go back and rebuild the object.
-include $(OBJS:.o:.d)
-include $(LIBK_OBJS:.o:.d)
-include $(LIBM_OBJS:.o:.d)