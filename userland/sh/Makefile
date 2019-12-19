OBJS=\
$(BUILDDIR)/userland/sh/builtin.o \
$(BUILDDIR)/userland/sh/command.o \
$(BUILDDIR)/userland/sh/input.o \
$(BUILDDIR)/userland/sh/job.o \
$(BUILDDIR)/userland/sh/main.o \
$(BUILDDIR)/userland/sh/parser.o

EXECUTABLE=$(BUILDDIR)/userland/sh/sh

GENERATED_FILES=\
generic_sh_parser.cpp \
generic_sh_parser.h \
sh_token_type.h

$(EXECUTABLE): $(GENERATED_FILES)

$(GENERATED_FILES): sh.y $(PARSER)
	$(PARSER) $< 2>/dev/null

include $(ROOT)/userland/Makefile.common