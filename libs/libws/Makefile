export ROOT?=../..

# Default host is given by script
DEFAULT_HOST!=../../default-host.sh
# If it's not already defined, HOST = DEFAULT_HOST
HOST?=$(DEFAULT_HOST)
# Host achitecture is given by script
HOSTARCH!=../../target-triplet-to-arch.sh $(HOST)

export ROOT?=$(CURDIR)/../..
export BUILDDIR?=$(ROOT)/build
export SYSROOT?=$(ROOT)/sysroot
export DESTDIR?=$(ROOT)/sysroot
export INCLUDEDIR?=/usr/include

export CC:=$(HOST)-gcc --sysroot=$(SYSROOT) -isystem=$(SYSROOT)/usr/include $(DEFINES)
export CXX:=$(HOST)-g++ --sysroot=$(SYSROOT) -isystem=$(SYSROOT)/usr/include $(DEFINES)
export AR:=$(HOST)-ar

export LIBRARY=$(BUILDDIR)/libs/libws/libws.a
export OBJS=\
$(BUILDDIR)/libs/libws/connection.o \
$(BUILDDIR)/libs/libws/window.o

include ../Makefile.common

$(LIBRARY): $(OBJS) $(MAKEFILE)
	cp $(BUILDDIR)/libs/libpthread/libpthread.a $(LIBRARY)
	$(AR) r $@ $^