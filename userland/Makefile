# Lists all projects
PROJECTS=cat chmod clear echo false grep head kill ls mkdir nslookup ping pwd rm rmdir sh sleep start stat touch true

ROOT?=$(CURDIR)/..
SYSROOT?=$(ROOT)/sysroot

# Init flags if not already set (can be overridden)
export CFLAGS?=
export CPPFLAGS?=-O2 -g -Werror -Wall -Wextra
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
