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

export LIBRARY=$(BUILDDIR)/libs/libpthread/libpthread.a
export OBJS=\
$(BUILDDIR)/libs/libpthread/atfork.o \
$(BUILDDIR)/libs/libpthread/attr.o \
$(BUILDDIR)/libs/libpthread/cancel.o \
$(BUILDDIR)/libs/libpthread/cond.o \
$(BUILDDIR)/libs/libpthread/condattr.o \
$(BUILDDIR)/libs/libpthread/getcpuclockid.o \
$(BUILDDIR)/libs/libpthread/key.o \
$(BUILDDIR)/libs/libpthread/mutex.o \
$(BUILDDIR)/libs/libpthread/mutexattr.o \
$(BUILDDIR)/libs/libpthread/once.o \
$(BUILDDIR)/libs/libpthread/pthread.o \
$(BUILDDIR)/libs/libpthread/spinlock.o

include ../Makefile.common
