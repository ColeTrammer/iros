# Default host is given by script
DEFAULT_HOST!=../../default-host.sh
# If it's not already defined, HOST = DEFAULT_HOST
HOST?=$(DEFAULT_HOST)
# Host achitecture is given by script
HOSTARCH!=../../target-triplet-to-arch.sh $(HOST)

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
$(BUILDDIR)/libs/libc/arpa/inet/inet.o \
$(BUILDDIR)/libs/libc/ctype/ctype.o \
$(BUILDDIR)/libs/libc/errno/errno.o \
$(BUILDDIR)/libs/libc/netinet/in.o \
$(BUILDDIR)/libs/libc/search/queue.o \
$(BUILDDIR)/libs/libc/stdio/printf.o \
$(BUILDDIR)/libs/libc/stdio/scanf.o \
$(BUILDDIR)/libs/libc/stdlib/abort.o \
$(BUILDDIR)/libs/libc/stdlib/conversions.o \
$(BUILDDIR)/libs/libc/stdlib/malloc.o \
$(BUILDDIR)/libs/libc/stdlib/stdlib.o \
$(BUILDDIR)/libs/libc/string/memchr.o \
$(BUILDDIR)/libs/libc/string/memcmp.o \
$(BUILDDIR)/libs/libc/string/memcpy.o \
$(BUILDDIR)/libs/libc/string/memmove.o \
$(BUILDDIR)/libs/libc/string/memset.o \
$(BUILDDIR)/libs/libc/string/strcat.o \
$(BUILDDIR)/libs/libc/string/strchr.o \
$(BUILDDIR)/libs/libc/string/strcmp.o \
$(BUILDDIR)/libs/libc/string/strcpy.o \
$(BUILDDIR)/libs/libc/string/strcspn.o \
$(BUILDDIR)/libs/libc/string/strdup.o \
$(BUILDDIR)/libs/libc/string/strlen.o \
$(BUILDDIR)/libs/libc/string/strncat.o \
$(BUILDDIR)/libs/libc/string/strncmp.o \
$(BUILDDIR)/libs/libc/string/strncpy.o \
$(BUILDDIR)/libs/libc/string/strpbrk.o \
$(BUILDDIR)/libs/libc/string/strrchr.o \
$(BUILDDIR)/libs/libc/string/strspn.o \
$(BUILDDIR)/libs/libc/string/strstr.o \
$(BUILDDIR)/libs/libc/string/strtok.o

# Lists objects used in 'hosted' enviornment
HOSTEDOBJS=\
$(ARCH_HOSTEDOBJS) \
$(BUILDDIR)/libs/libc/assert/assert.o \
$(BUILDDIR)/libs/libc/dirent/dirent.o \
$(BUILDDIR)/libs/libc/ftw/ftw.o \
$(BUILDDIR)/libs/libc/locale/locale.o \
$(BUILDDIR)/libs/libc/glob/glob.o \
$(BUILDDIR)/libs/libc/netdb/netdb.o \
$(BUILDDIR)/libs/libc/pwd/pwd.o \
$(BUILDDIR)/libs/libc/signal/signal.o \
$(BUILDDIR)/libs/libc/stdio/files.o \
$(BUILDDIR)/libs/libc/stdio/puts.o \
$(BUILDDIR)/libs/libc/stdlib/atexit.o \
$(BUILDDIR)/libs/libc/stdlib/env.o \
$(BUILDDIR)/libs/libc/string/strcoll.o \
$(BUILDDIR)/libs/libc/string/strerror.o \
$(BUILDDIR)/libs/libc/string/strxfrm.o \
$(BUILDDIR)/libs/libc/sys/socket/socket.o \
$(BUILDDIR)/libs/libc/sys/wait/wait.o \
$(BUILDDIR)/libs/libc/termios/termios.o \
$(BUILDDIR)/libs/libc/time/time.o \
$(BUILDDIR)/libs/libc/unistd/exec.o \
$(BUILDDIR)/libs/libc/unistd/getlogin.o \
$(BUILDDIR)/libs/libc/unistd/getopt.o \
$(BUILDDIR)/libs/libc/unistd/isatty.o \
$(BUILDDIR)/libs/libc/wordexp/wordexp.o \
$(BUILDDIR)/libs/libc/init.o

# Lists all objects
OBJS=\
$(FREEOBJS) \
$(HOSTEDOBJS)

# Lists objects used by libk
LIBK_OBJS=$(FREEOBJS:.o=.libk.o)

# List of binaries to be created
BINARIES=$(BUILDDIR)/libs/libc/libc.a $(BUILDDIR)/libs/libc/libk.a

.PHONY: all
all: $(BINARIES)

# Makes library file from objs using AR
$(BUILDDIR)/libs/libc/libc.a: $(OBJS)
	$(AR) rcs $@ $^

$(BUILDDIR)/libs/libc/libk.a: $(LIBK_OBJS)
	$(AR) rcs $@ $^

# Makes object file from any .c or .S file
$(BUILDDIR)/%.o: $(ROOT)/%.c
	$(CC) -MD -c $< -o $@ $(CFLAGS) $(CPPFLAGS)

$(BUILDDIR)/%.o: $(ROOT)/%.S
	$(CC) -c $< -o $@ $(CFLAFS) $(CPPFLAGS)

$(BUILDDIR)/%.libk.o: $(ROOT)/%.c
	$(CC) -MD -c $< -o $@ $(LIBK_CFLAGS) $(LIBK_CPPFLAGS)

$(BUILDDIR)/%.libk.o: $(ROOT)/%.S
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