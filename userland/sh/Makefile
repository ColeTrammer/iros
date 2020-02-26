export ROOT?=../..
export BUILDDIR?=$(ROOT)/build

OBJS=\
$(BUILDDIR)/userland/sh/builtin.o \
$(BUILDDIR)/userland/sh/command.o \
$(BUILDDIR)/userland/sh/input.o \
$(BUILDDIR)/userland/sh/generic_sh_parser.o \
$(BUILDDIR)/userland/sh/job.o \
$(BUILDDIR)/userland/sh/main.o \
$(BUILDDIR)/userland/sh/sh_lexer.o \
$(BUILDDIR)/userland/sh/sh_state.o

NATIVE_OBJS=\
$(BUILDDIR)/libs/libc/fnmatch/fnmatch.o \
$(BUILDDIR)/libs/libc/glob/glob.o \
$(BUILDDIR)/libs/libc/pwd/pwd.o \
$(BUILDDIR)/libs/libc/wordexp/wordexp.o

EXECUTABLE=$(BUILDDIR)/userland/sh/sh

GENERATED_FILES=\
generic_sh_parser_impl.cpp \
generic_sh_parser.h \
sh_token_type.h \
sh.info

$(EXECUTABLE) $(OBJS): $(GENERATED_FILES)

$(GENERATED_FILES): sh.y
	$(PARSER) $< 2>sh.info

export LD=$(CXX)

include $(ROOT)/userland/Makefile.common

clean:
	rm -f $(GENERATED_FILES)