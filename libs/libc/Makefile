# Default host is given by script
DEFAULT_HOST!=../../default-host.sh
# If it's not already defined, HOST = DEFAULT_HOST
HOST?=$(DEFAULT_HOST)
# Host achitecture is given by script
HOSTARCH!=../../target-triplet-to-arch.sh $(HOST)

export ROOT?=$(CURDIR)/../..
export SYSROOT?=$(ROOT)/sysroot
export BUILDDIR?=$(ROOT)/build

export CC:=$(HOST)-gcc --sysroot=$(SYSROOT) -isystem=$(SYSROOT)/usr/include $(DEFINES)
export CXX:=$(HOST)-g++ --sysroot=$(SYSROOT) -isystem=$(SYSROOT)/usr/include $(DEFINES) -std=c++2a
export AR:=$(HOST)-ar

# Sets directories for install if not already set
export DESTDIR?=$(SYSROOT)
export PREFIX?=/usr
export EXEC_PREFIX?=$(PREFIX)
export INCLUDEDIR?=$(PREFIX)/include
export LIBDIR?=$(EXEC_PREFIX)/lib

# Init flags if not already set (can be overridden)
CFLAGS?=-O2 -g -DARCH=$(HOSTARCH)
CPPFLAGS?=
LDFLAGS?=
LIBS?=

# Include options that should always be set
CFLAGS:=$(CFLAGS) -ffreestanding -Werror -Wall -Wextra -std=gnu2x
CPPFLAGS:=$(CPPFLAGS) -D__is_libc -Iinclude -D_OS_2_SOURCE
LIBK_CFLAGS:=$(CFLAGS)
LIBK_CPPFLAGS:=$(CPPFLAGS) -D__is_libk

# Include architecture specific options
ARCHDIR=arch/$(HOSTARCH)
include $(ARCHDIR)/make.config

# Include regex specific rules
include $(ROOT)/libs/libc/regex/make.config

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
$(BUILDDIR)/libs/libc/string/stpcpy.o \
$(BUILDDIR)/libs/libc/string/stpncpy.o \
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
$(BUILDDIR)/libs/libc/string/strndup.o \
$(BUILDDIR)/libs/libc/string/strnlen.o \
$(BUILDDIR)/libs/libc/string/strpbrk.o \
$(BUILDDIR)/libs/libc/string/strrchr.o \
$(BUILDDIR)/libs/libc/string/strspn.o \
$(BUILDDIR)/libs/libc/string/strstr.o \
$(BUILDDIR)/libs/libc/string/strtok_r.o \
$(BUILDDIR)/libs/libc/string/strtok.o \
$(BUILDDIR)/libs/libc/strings/strings.o

# Lists objects used in 'hosted' enviornment
HOSTEDOBJS=\
$(ARCH_HOSTEDOBJS) \
$(BUILDDIR)/libs/libc/assert/assert.o \
$(BUILDDIR)/libs/libc/bits/lock/__lock.o \
$(BUILDDIR)/libs/libc/bits/lock/__trylock.o \
$(BUILDDIR)/libs/libc/bits/lock/__unlock.o \
$(BUILDDIR)/libs/libc/dirent/dirent.o \
$(BUILDDIR)/libs/libc/fcntl/creat.o \
$(BUILDDIR)/libs/libc/fcntl/fcntl.o \
$(BUILDDIR)/libs/libc/fcntl/open.o \
$(BUILDDIR)/libs/libc/fcntl/openat.o \
$(BUILDDIR)/libs/libc/fnmatch/fnmatch.o \
$(BUILDDIR)/libs/libc/ftw/ftw.o \
$(BUILDDIR)/libs/libc/glob/glob.o \
$(BUILDDIR)/libs/libc/grp/grp.o \
$(BUILDDIR)/libs/libc/libgen/basename.o \
$(BUILDDIR)/libs/libc/libgen/dirname.o \
$(BUILDDIR)/libs/libc/locale/locale.o \
$(BUILDDIR)/libs/libc/netdb/netdb.o \
$(BUILDDIR)/libs/libc/pwd/pwd.o \
$(REGEX_OBJS) \
$(BUILDDIR)/libs/libc/signal/signal.o \
$(BUILDDIR)/libs/libc/stdio/clearerr_unlocked.o \
$(BUILDDIR)/libs/libc/stdio/clearerr.o \
$(BUILDDIR)/libs/libc/stdio/fclose.o \
$(BUILDDIR)/libs/libc/stdio/fdopen.o \
$(BUILDDIR)/libs/libc/stdio/feof_unlocked.o \
$(BUILDDIR)/libs/libc/stdio/feof.o \
$(BUILDDIR)/libs/libc/stdio/ferror_unlocked.o \
$(BUILDDIR)/libs/libc/stdio/ferror.o \
$(BUILDDIR)/libs/libc/stdio/fflush_unlocked.o \
$(BUILDDIR)/libs/libc/stdio/fflush.o \
$(BUILDDIR)/libs/libc/stdio/fgetc_unlocked.o \
$(BUILDDIR)/libs/libc/stdio/fgetc.o \
$(BUILDDIR)/libs/libc/stdio/fgetpos.o \
$(BUILDDIR)/libs/libc/stdio/fgets_unlocked.o \
$(BUILDDIR)/libs/libc/stdio/fgets.o \
$(BUILDDIR)/libs/libc/stdio/fileno_unlocked.o \
$(BUILDDIR)/libs/libc/stdio/fileno.o \
$(BUILDDIR)/libs/libc/stdio/files.o \
$(BUILDDIR)/libs/libc/stdio/flockfile.o \
$(BUILDDIR)/libs/libc/stdio/fopen_internal.o \
$(BUILDDIR)/libs/libc/stdio/fopen.o \
$(BUILDDIR)/libs/libc/stdio/fputc_unlocked.o \
$(BUILDDIR)/libs/libc/stdio/fputc.o \
$(BUILDDIR)/libs/libc/stdio/fputs_unlocked.o \
$(BUILDDIR)/libs/libc/stdio/fputs.o \
$(BUILDDIR)/libs/libc/stdio/fread_unlocked.o \
$(BUILDDIR)/libs/libc/stdio/fread.o \
$(BUILDDIR)/libs/libc/stdio/freopen.o \
$(BUILDDIR)/libs/libc/stdio/fseek_unlocked.o \
$(BUILDDIR)/libs/libc/stdio/fseek.o \
$(BUILDDIR)/libs/libc/stdio/fsetpos.o \
$(BUILDDIR)/libs/libc/stdio/ftell.o \
$(BUILDDIR)/libs/libc/stdio/ftrylockfile.o \
$(BUILDDIR)/libs/libc/stdio/funlockfile.o \
$(BUILDDIR)/libs/libc/stdio/fwrite_unlocked.o \
$(BUILDDIR)/libs/libc/stdio/fwrite.o \
$(BUILDDIR)/libs/libc/stdio/getc_unlocked.o \
$(BUILDDIR)/libs/libc/stdio/getc.o \
$(BUILDDIR)/libs/libc/stdio/getchar_unlocked.o \
$(BUILDDIR)/libs/libc/stdio/getchar.o \
$(BUILDDIR)/libs/libc/stdio/getdelim.o \
$(BUILDDIR)/libs/libc/stdio/getline.o \
$(BUILDDIR)/libs/libc/stdio/gets.o \
$(BUILDDIR)/libs/libc/stdio/init_files.o \
$(BUILDDIR)/libs/libc/stdio/putc_unlocked.o \
$(BUILDDIR)/libs/libc/stdio/putc.o \
$(BUILDDIR)/libs/libc/stdio/putchar_unlocked.o \
$(BUILDDIR)/libs/libc/stdio/putchar.o \
$(BUILDDIR)/libs/libc/stdio/puts.o \
$(BUILDDIR)/libs/libc/stdio/rewind.o \
$(BUILDDIR)/libs/libc/stdio/setbuf.o \
$(BUILDDIR)/libs/libc/stdio/setvbuf.o \
$(BUILDDIR)/libs/libc/stdio/unget.o \
$(BUILDDIR)/libs/libc/stdlib/atexit.o \
$(BUILDDIR)/libs/libc/stdlib/env.o \
$(BUILDDIR)/libs/libc/stdlib/init_env.o \
$(BUILDDIR)/libs/libc/stdlib/realpath.o \
$(BUILDDIR)/libs/libc/string/strcoll.o \
$(BUILDDIR)/libs/libc/string/strerror.o \
$(BUILDDIR)/libs/libc/string/strxfrm.o \
$(BUILDDIR)/libs/libc/sys/ioctl/ioctl.o \
$(BUILDDIR)/libs/libc/sys/mman/mman.o \
$(BUILDDIR)/libs/libc/sys/os_2/create_task.o \
$(BUILDDIR)/libs/libc/sys/os_2/exit_task.o \
$(BUILDDIR)/libs/libc/sys/os_2/get_initial_process_info.o \
$(BUILDDIR)/libs/libc/sys/os_2/os_mutex.o \
$(BUILDDIR)/libs/libc/sys/os_2/set_thread_self_pointer.o \
$(BUILDDIR)/libs/libc/sys/os_2/tgkill.o \
$(BUILDDIR)/libs/libc/sys/socket/socket.o \
$(BUILDDIR)/libs/libc/sys/select/select.o \
$(BUILDDIR)/libs/libc/sys/stat/stat.o \
$(BUILDDIR)/libs/libc/sys/syscall/syscall_to_string.o \
$(BUILDDIR)/libs/libc/sys/time/time.o \
$(BUILDDIR)/libs/libc/sys/times/times.o \
$(BUILDDIR)/libs/libc/sys/uio/readv.o \
$(BUILDDIR)/libs/libc/sys/uio/writev.o \
$(BUILDDIR)/libs/libc/sys/utsname/utsname.o \
$(BUILDDIR)/libs/libc/sys/wait/wait.o \
$(BUILDDIR)/libs/libc/termios/termios.o \
$(BUILDDIR)/libs/libc/time/time.o \
$(BUILDDIR)/libs/libc/unistd/_exit.o \
$(BUILDDIR)/libs/libc/unistd/close.o \
$(BUILDDIR)/libs/libc/unistd/exec.o \
$(BUILDDIR)/libs/libc/unistd/fork.o \
$(BUILDDIR)/libs/libc/unistd/getlogin.o \
$(BUILDDIR)/libs/libc/unistd/getopt.o \
$(BUILDDIR)/libs/libc/unistd/isatty.o \
$(BUILDDIR)/libs/libc/unistd/lseek.o \
$(BUILDDIR)/libs/libc/unistd/pread.o \
$(BUILDDIR)/libs/libc/unistd/pwrite.o \
$(BUILDDIR)/libs/libc/unistd/read.o \
$(BUILDDIR)/libs/libc/unistd/sbrk.o \
$(BUILDDIR)/libs/libc/unistd/tty_stuff.o \
$(BUILDDIR)/libs/libc/unistd/unistd.o \
$(BUILDDIR)/libs/libc/unistd/write.o \
$(BUILDDIR)/libs/libc/utime/utime.o \
$(BUILDDIR)/libs/libc/wordexp/wordexp.o \
$(BUILDDIR)/libs/libc/cxx.o \
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

$(BUILDDIR)/%.o: $(ROOT)/%.cpp
	$(CXX) -MD -c $< -o $@ $(CPPFLAGS) -fno-rtti -fno-exceptions -Werror -Wall -Wextra -ffreestanding -nostdinc++ -fconcepts

$(BUILDDIR)/%.o: $(ROOT)/%.S
	$(CC) -c $< -o $@ $(CFLAFS) $(CPPFLAGS)

$(BUILDDIR)/%.libk.o: $(ROOT)/%.c
	$(CC) -MD -c $< -o $@ $(LIBK_CFLAGS) $(LIBK_CPPFLAGS)

$(BUILDDIR)/%.libk.o: $(ROOT)/%.cpp
	$(CXX) -MD -c $< -o $@ $(LIBK_CPPFLAGS)

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