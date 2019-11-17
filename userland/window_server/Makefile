OBJS=\
$(BUILDDIR)/userland/window_server/main.o \
$(BUILDDIR)/userland/window_server/server.o \
$(BUILDDIR)/userland/window_server/window.o

EXECUTABLE=$(BUILDDIR)/userland/window_server/window_server

CPP_FLAGS+=-fno-exceptions
LDFLAGS+=-lgraphics
export LD=$(CXX)

.PHONY: install-headers
install-headers:
	cp -R --preserve=timestamps -u $(ROOT)/userland/window_server/include/. $(SYSROOT)/usr/include

include $(ROOT)/userland/Makefile.common