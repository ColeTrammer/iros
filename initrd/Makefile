# Init flags if not already set (can be overridden)
CFLAGS?=-O2 -g
CPPFLAGS?=
LDFLAGS?=
LIBS?=

# Include options that should always be set
CFLAGS:=$(CFLAGS) -Werror -Wall -Wextra -D_OS_2_SOURCE
CPPFLAGS:=$(CPPFLAGS)
LDFLAFS:=$(LDFLAGS) -T $(ROOT)/ports/kilo/kilo/link.ld
CXX_LIBS:=$(CXX_LIBS) -lc -lgraphics
C_LIBS:=$(C_LIBC) -lpthread

# Sets directories for install if not already set
DESTDIR?=
PREFIX?=
EXEC_PREFIX?=$(PREFIX)
BOOTDIR?=$(EXEC_PREFIX)/boot

# Lists all objects - includes extra objects needed for standard c programs
OBJS=\
$(BUILDDIR)/initrd/initrd.o

# Lists all files included in initrd
FILES:=$(shell find files)

# Lists all programs included in initrd
PROGRAMS=\
$(BUILDDIR)/initrd/programs/alarm_test \
$(BUILDDIR)/initrd/programs/aligned_alloc_test \
$(BUILDDIR)/initrd/programs/broken \
$(BUILDDIR)/initrd/programs/fwrite \
$(BUILDDIR)/initrd/programs/getopt_test \
$(BUILDDIR)/initrd/programs/glob_test \
$(BUILDDIR)/initrd/programs/guess \
$(BUILDDIR)/initrd/programs/hash_map_test \
$(BUILDDIR)/initrd/programs/linked_list_test_a \
$(BUILDDIR)/initrd/programs/local_socket_client_test \
$(BUILDDIR)/initrd/programs/local_socket_server_test \
$(BUILDDIR)/initrd/programs/mmap_test \
$(BUILDDIR)/initrd/programs/nftw_test \
$(BUILDDIR)/initrd/programs/pipe_test \
$(BUILDDIR)/initrd/programs/queue_test \
$(BUILDDIR)/initrd/programs/scanf_test \
$(BUILDDIR)/initrd/programs/serial \
$(BUILDDIR)/initrd/programs/set_jump_test \
$(BUILDDIR)/initrd/programs/shared_memory_test \
$(BUILDDIR)/initrd/programs/signal_test \
$(BUILDDIR)/initrd/programs/sigsuspend_test_a \
$(BUILDDIR)/initrd/programs/smart_pointer_test \
$(BUILDDIR)/initrd/programs/stack_trace_test \
$(BUILDDIR)/initrd/programs/strcasecmp_test \
$(BUILDDIR)/initrd/programs/string_test \
$(BUILDDIR)/initrd/programs/strtoken_test \
$(BUILDDIR)/initrd/programs/tcp_server_test_a \
$(BUILDDIR)/initrd/programs/tcp_test \
$(BUILDDIR)/initrd/programs/te \
$(BUILDDIR)/initrd/programs/test_cpp \
$(BUILDDIR)/initrd/programs/test_exec \
$(BUILDDIR)/initrd/programs/test \
$(BUILDDIR)/initrd/programs/thread_test \
$(BUILDDIR)/initrd/programs/times_test \
$(BUILDDIR)/initrd/programs/tmpnam_test \
$(BUILDDIR)/initrd/programs/udp_test \
$(BUILDDIR)/initrd/programs/vector_test \
$(BUILDDIR)/initrd/programs/vga_test_a \
$(BUILDDIR)/initrd/programs/we_test \
$(BUILDDIR)/initrd/programs/window_server_test

.PHONY: all
all: $(BUILDDIR)/initrd/initrd.bin

.PHONY: make-output-dir
make-output-dir:
	mkdir -p $(ROOT)/initrd/files

# Builds initrd.img from generated tool
$(BUILDDIR)/initrd/initrd.bin: $(BUILDDIR)/initrd/initrd.o make-output-dir $(FILES) $(PROGRAMS)
	for program in $(PROGRAMS); do \
	  cp $$program files; \
	done
	$< files $@

# Builds any .o file from its .c, and generates its makefile dependencies with -MD
$(BUILDDIR)/initrd/initrd.o: $(ROOT)/initrd/initrd.c
	gcc -MD -o $@ $< $(CFLAGS) $(CPPFLAGS) -Wno-stringop-truncation

$(BUILDDIR)/%: $(ROOT)/%.S
	$(CC) -MD -o $@ $< $(CFLAGS) $(CPPFLAGS) $(LDFLAFS)

$(BUILDDIR)/%: $(ROOT)/%.c
	$(CC) -MD -o $@ $< $(CFLAGS) $(CPPFLAGS) $(LDFLAFS) $(C_LIBS)

$(BUILDDIR)/%: $(ROOT)/%.cpp
	$(CXX) -MD -o $@ $< $(CPPFLAGS) $(LDFLAGS) $(CXX_LIBS)

# Cleans by deleting .o, and .d files
.PHONY: clean
clean:
	rm -f initrd.bin
	rm -f $(OBJS)
	rm -f $(OBJS:.o=.d) 

.PHONY: install install-initrd

install: install-initrd

# Installs initrd via cp
install-initrd: $(BUILDDIR)/initrd/initrd.bin
	mkdir -p $(DESTDIR)$(BOOTDIR)
	cp --preserve=timestamps $< $(DESTDIR)$(BOOTDIR)

# Includes the .d files for each object, if it exists.
# This will cause any object that depends on a specific header,
# that was changed, to be recompiled, as this include will 
# tell make its dependencies, which if altered, will cause
# make to go back and rebuild the object.
-include $(OBJS:.o=.d)