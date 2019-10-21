# Lists all projects
PROJECTS=sh

ROOT?=$(CURDIR)/..
SYSROOT?=$(ROOT)/sysroot

# Init flags if not already set (can be overridden)
export CFLAGS?=-O2 -g
export CPPFLAGS?=
export LDFLAGS+=-T $(ROOT)/userland/link.ld
export LIBS?=

.PHONY: all
all: $(PROJECTS)

.PHONY: install
install: all

# Makes project by calling its Makefile
.PHONY: $(PROJECTS)
$(PROJECTS):
	$(MAKE) install -C $(ROOT)/userland/$@
