# Init flags if not already set (can be overridden)
CFLAGS?=-O2
CPPFLAGS?=
LDFLAGS?=
LIBS?=

# Include options that should always be set
CFLAGS:=$(CFLAGS)
CPPFLAGS:=$(CPPFLAGS) -DARCH=$(HOSTARCH) -Werror -Wall -Wextra -D_OS_2_SOURCE
CXX_LIBS:=$(CXX_LIBS) -lc -lws -lgraphics -lrt
C_LIBS:=$(C_LIBS) -lrt

# Sets directories for install if not already set
DESTDIR?=
PREFIX?=
EXEC_PREFIX?=$(PREFIX)
BOOTDIR?=$(EXEC_PREFIX)/boot

# Lists all files included in initrd
FILES:=$(shell find files)

# Lists all programs included in initrd
PROGRAMS=\
$(BUILDDIR)/initrd/programs/alarm_test \
$(BUILDDIR)/initrd/programs/aligned_alloc_test \
$(BUILDDIR)/initrd/programs/broken \
$(BUILDDIR)/initrd/programs/cancel_test \
$(BUILDDIR)/initrd/programs/cond_test \
$(BUILDDIR)/initrd/programs/cleanup_handler_test \
$(BUILDDIR)/initrd/programs/detached_thread_test \
$(BUILDDIR)/initrd/programs/fault_test \
$(BUILDDIR)/initrd/programs/fread_test \
$(BUILDDIR)/initrd/programs/fwrite \
$(BUILDDIR)/initrd/programs/getcpuclockid_test \
$(BUILDDIR)/initrd/programs/getopt_test \
$(BUILDDIR)/initrd/programs/glob_test \
$(BUILDDIR)/initrd/programs/guess \
$(BUILDDIR)/initrd/programs/hash_map_test \
$(BUILDDIR)/initrd/programs/linked_list_test_a \
$(BUILDDIR)/initrd/programs/local_socket_client_test \
$(BUILDDIR)/initrd/programs/local_socket_server_test \
$(BUILDDIR)/initrd/programs/map_shared_test \
$(BUILDDIR)/initrd/programs/mmap_test \
$(BUILDDIR)/initrd/programs/mutex_test \
$(BUILDDIR)/initrd/programs/nftw_test \
$(BUILDDIR)/initrd/programs/once_test \
$(BUILDDIR)/initrd/programs/pipe_test \
$(BUILDDIR)/initrd/programs/protections_test \
$(BUILDDIR)/initrd/programs/pthread_atfork_test \
$(BUILDDIR)/initrd/programs/pthread_attr_test \
$(BUILDDIR)/initrd/programs/queue_test \
$(BUILDDIR)/initrd/programs/regex_test \
$(BUILDDIR)/initrd/programs/scanf_test \
$(BUILDDIR)/initrd/programs/select_test \
$(BUILDDIR)/initrd/programs/serial \
$(BUILDDIR)/initrd/programs/set_jump_test \
$(BUILDDIR)/initrd/programs/shared_memory_test \
$(BUILDDIR)/initrd/programs/sigaltstack_test \
$(BUILDDIR)/initrd/programs/siginfo_test \
$(BUILDDIR)/initrd/programs/signal_test \
$(BUILDDIR)/initrd/programs/sigqueue_test \
$(BUILDDIR)/initrd/programs/sigsuspend_test_a \
$(BUILDDIR)/initrd/programs/smart_pointer_test \
$(BUILDDIR)/initrd/programs/stack_trace_test \
$(BUILDDIR)/initrd/programs/strcasecmp_test \
$(BUILDDIR)/initrd/programs/string_test \
$(BUILDDIR)/initrd/programs/strtoken_test \
$(BUILDDIR)/initrd/programs/tcp_server_test_a \
$(BUILDDIR)/initrd/programs/tcp_test_a \
$(BUILDDIR)/initrd/programs/te \
$(BUILDDIR)/initrd/programs/test_cpp \
$(BUILDDIR)/initrd/programs/test_exec \
$(BUILDDIR)/initrd/programs/test_regex \
$(BUILDDIR)/initrd/programs/test \
$(BUILDDIR)/initrd/programs/tgkill_test \
$(BUILDDIR)/initrd/programs/thread_specific_data_test \
$(BUILDDIR)/initrd/programs/thread_test \
$(BUILDDIR)/initrd/programs/timer_overrun_test \
$(BUILDDIR)/initrd/programs/timer_test \
$(BUILDDIR)/initrd/programs/times_test \
$(BUILDDIR)/initrd/programs/tls_test \
$(BUILDDIR)/initrd/programs/tmpnam_test \
$(BUILDDIR)/initrd/programs/udp_test \
$(BUILDDIR)/initrd/programs/vector_test \
$(BUILDDIR)/initrd/programs/vga_test_a \
$(BUILDDIR)/initrd/programs/we_test \
$(BUILDDIR)/initrd/programs/window_server_test

.PHONY: all
all: $(BUILDDIR)/initrd/initrd.bin

$(PROGRAMS): $(SYSROOT)/usr/lib/libc.a $(SYSROOT)/usr/lib/libpthread.a $(SYSROOT)/usr/lib/libgraphics.a

.PHONY: make-output-dir
make-output-dir:
	mkdir -p $(ROOT)/initrd/files

# Builds initrd.img from generated tool
$(BUILDDIR)/initrd/initrd.bin: make-output-dir $(FILES) $(PROGRAMS)
	for program in $(PROGRAMS); do \
	  cp $$program files; \
	done
	$(INITRD) files $@

$(BUILDDIR)/%: $(ROOT)/%.S
	$(CC) -MD -o $@ $< $(CFLAGS) $(CPPFLAGS) $(LDFLAGS)

$(BUILDDIR)/%: $(ROOT)/%.c
	$(CC) -MD -o $@ $< $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) $(C_LIBS)

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