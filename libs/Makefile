LIBS=libc libgraphics libm libpthread
HEADER_LIBS=$(LIBS) libliim

export ROOT?=$(CURDIR)/..
export SYSROOT?=$(ROOT)/sysroot

# Sets directories for install if not already set
export DESTDIR?=
export PREFIX?=/usr
export EXEC_PREFIX?=$(PREFIX)
export INCLUDEDIR?=$(PREFIX)/include
export LIBDIR?=$(EXEC_PREFIX)/lib

# Init flags if not already set (can be overridden)
export CFLAGS?=-std=gnu11 -D_OS_2_SOURCE
export CPPFLAGS?=-O2 -g -Werror -Wall -Wextra
export LDFLAGS+=-T $(ROOT)/userland/link.ld

.PHONY: all
all: $(LIBS)

.PHONY: install
install: all

.PHONY: install-headers
install-headers:
	for lib in $(HEADER_LIBS); do \
	    $(MAKE) install-headers -C $(ROOT)/libs/$$lib; \
	done

# Makes project by calling its Makefile
.PHONY: $(LIBS)
$(LIBS):
	$(MAKE) install -C $(ROOT)/libs/$@