OBJS=\
$(BUILDDIR)/userland/window_server/main.o \

EXECUTABLE=$(BUILDDIR)/userland/window_server/window_server

CPP_FLAGS+=-fno-exceptions
LDFLAGS+=-lgraphics
export LD=$(CXX)

include $(ROOT)/userland/Makefile.common