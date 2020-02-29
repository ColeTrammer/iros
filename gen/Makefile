# Lists all projects
PROJECTS=parser
HEADER_PROJECTS=parser
NATIVE_PROJECTS=parser

export ROOT?=$(CURDIR)/..
export SYSROOT?=$(ROOT)/sysroot

# Init flags if not already set (can be overridden)
export CFLAGS?=-std=gnu2x
export CPPFLAGS?=-O2 -Werror -Wall -Wextra -fconcepts
export LDFLAGS?=
export LIBS?=

.PHONY: all
all: $(PROJECTS)

.PHONY: install
install: all

.PHONY: native
native: install-headers
	for proj in $(NATIVE_PROJECTS); do \
	  $(MAKE) native -C $(ROOT)/gen/$$proj; \
	done

.PHONY: install-headers
install-headers:
	for proj in $(HEADER_PROJECTS); do \
	  $(MAKE) install-headers -C $(ROOT)/gen/$$proj; \
	done

# Makes project by calling its Makefile
.PHONY: $(PROJECTS)
$(PROJECTS):
	$(MAKE) install -C $(ROOT)/gen/$@

.PHONY: clean
clean:
	for proj in $(PROJECTS); do \
	  $(MAKE) clean -C $(ROOT)/gen/$$proj; \
	done