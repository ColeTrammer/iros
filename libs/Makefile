LIBS=libc libm

export ROOT?=$(CURDIR)/..
export SYSROOT?=$(ROOT)/sysroot

# Sets directories for install if not already set
export DESTDIR?=
export PREFIX?=/usr
export EXEC_PREFIX?=$(PREFIX)
export INCLUDEDIR?=$(PREFIX)/include
export LIBDIR?=$(EXEC_PREFIX)/lib

# Init flags if not already set (can be overridden)
export CFLAGS?=-std=gnu11
export CPPFLAGS?=-O2 -g -Werror -Wall -Wextra
export LDFLAGS+=-T $(ROOT)/userland/link.ld

.PHONY: all
all: $(LIBS)

.PHONY: install
install: all

.PHONY: install-headers
install-headers:
	for lib in $(LIBS); do \
	    $(MAKE) install -C $(ROOT)/libs/$$lib; \
	done

# Makes project by calling its Makefile
.PHONY: $(LIBS)
$(LIBS):
	$(MAKE) install -C $(ROOT)/libs/$@