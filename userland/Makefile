# Lists all projects
PROJECTS=basename cat chmod chown clear cp date dirname echo env false grep head kill ln ls mkdir mv nslookup ping pwd realpath rm rmdir sh sleep start stat touch true window_server xterm
HEADER_PROJECTS=window_server

export ROOT?=$(CURDIR)/..
export SYSROOT?=$(ROOT)/sysroot

# Init flags if not already set (can be overridden)
export CFLAGS?=
export CPPFLAGS?=-O2 -g -Werror -Wall -Wextra
export LDFLAGS+=-T $(ROOT)/ports/kilo/kilo/link.ld
export LIBS?=

.PHONY: all
all: $(PROJECTS)

.PHONY: install
install: all

# Makes project by calling its Makefile
.PHONY: $(PROJECTS)
$(PROJECTS):
	$(MAKE) install -C $(ROOT)/userland/$@

.PHONY: install-headers
install-headers:
	for proj in $(HEADER_PROJECTS); do \
	  $(MAKE) install-headers -C $(ROOT)/userland/$$proj; \
	done
