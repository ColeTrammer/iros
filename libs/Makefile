LIBS=libc libgraphics libm libpthread librt libws
HEADER_LIBS=$(LIBS) libliim

export ROOT?=$(CURDIR)/..
export SYSROOT?=$(ROOT)/sysroot

# Sets directories for install if not already set
export DESTDIR?=$(ROOT)/sysroot
export PREFIX?=/usr
export EXEC_PREFIX?=$(PREFIX)
export INCLUDEDIR?=$(PREFIX)/include
export LIBDIR?=$(EXEC_PREFIX)/lib
export BUILDDIR?=$(ROOT)/build

export CFLAGS+=-std=gnu2x
export CPPFLAGS+=-O2 -g -Werror -Wall -Wextra -DARCH=$(HOSTARCH) -D_OS_2_SOURCE -D__is_libc
export LDFLAGS+=-T $(ROOT)/userland/link.ld

.PHONY: all
all: $(LIBS)

.PHONY: install
install: all

libc: libpthread
librt: libpthread

.PHONY: install-headers
install-headers:
	for lib in $(HEADER_LIBS); do \
	  $(MAKE) install-headers -C $(ROOT)/libs/$$lib; \
	done

.PHONY: clean
clean:
	for lib in $(LIBS); do \
	  $(MAKE) clean -C $(ROOT)/libs/$$lib; \
	done

# Makes project by calling its Makefile
.PHONY: $(LIBS)
$(LIBS):
	$(MAKE) install -C $(ROOT)/libs/$@