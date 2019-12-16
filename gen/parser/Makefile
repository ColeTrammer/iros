export ROOT?=../../
export BUILDDIR?=$(ROOT)/build

OBJS=\
$(BUILDDIR)/gen/parser/extended_grammar.o \
$(BUILDDIR)/gen/parser/item_set.o \
$(BUILDDIR)/gen/parser/lexer.o \
$(BUILDDIR)/gen/parser/main.o

EXECUTABLE=$(BUILDDIR)/gen/parser/parser

export LD=$(CXX)
export CPPFLAGS+=-I$(ROOT)/libs/libliim/include -std=c++17

include $(ROOT)/gen/Makefile.common