export ROOT?=../..
export BUILDDIR?=$(ROOT)/build
export PROJECT_NAME=initrd

OBJS=\
$(BUILDDIR)/gen/initrd/main.o

EXECUTABLE=$(BUILDDIR)/gen/initrd/initrd

export CFLAGS+=-Wno-stringop-truncation

include $(ROOT)/gen/Makefile.common