# Init flags if not already set (can be overridden)
CFLAGS?=-O2 -g
CPPFLAGS?=
LDFLAGS?=
LIBS?=

# Include options that should always be set
CFLAGS:=$(CFLAGS) -Wall -Wextra -std=gnu11
CPPFLAGS:=$(CPPFLAGS)
LDFLAFS:=$(LDFLAGS)
LIBS:=$(LIBS)

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
$(BUILDDIR)/initrd/programs/cat \
$(BUILDDIR)/initrd/programs/clear \
$(BUILDDIR)/initrd/programs/ls \
$(BUILDDIR)/initrd/programs/serial \
$(BUILDDIR)/initrd/programs/shell \
$(BUILDDIR)/initrd/programs/test \
$(BUILDDIR)/initrd/programs/test_exec

.PHONY: all
all: $(BUILDDIR)/initrd/initrd.bin

# Builds initrd.img from generated tool
$(BUILDDIR)/initrd/initrd.bin: $(BUILDDIR)/initrd/initrd.o $(FILES) $(PROGRAMS)
	for program in $(PROGRAMS); do \
	  cp $$program files; \
	done
	$< files $@

# Builds any .o file from its .c, and generates its makefile dependencies with -MD
$(BUILDDIR)/initrd/initrd.o: $(BUILDDIR)/initrd/initrd.c
	gcc -MD -o $@ $< $(CFLAGS) $(CPPFLAGS)

%: %.S
	$(CC) -MD -o $@ $< $(CFLAGS) $(CPPFLAGS) -T programs/link.ld -mno-mmx -mno-sse -mno-sse2

%: %.c
	$(CC) -MD -o $@ $< $(CFLAGS) $(CPPFLAGS) -T programs/link.ld -mno-mmx -mno-sse -mno-sse2

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