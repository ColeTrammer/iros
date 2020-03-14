OBJS=\
$(BUILDDIR)/userland/xterm/main.o \
$(BUILDDIR)/userland/xterm/raw_frame_buffer_wrapper.o \
$(BUILDDIR)/userland/xterm/terminal.o \
$(BUILDDIR)/userland/xterm/tty.o \
$(BUILDDIR)/userland/xterm/vga_buffer.o \
$(BUILDDIR)/userland/xterm/window_wrapper.o

EXECUTABLE=$(BUILDDIR)/userland/xterm/xterm

CPP_FLAGS+=-fno-exceptions
LDFLAGS+=-lws -lgraphics -lrt
export LD=$(CXX)

include $(ROOT)/userland/Makefile.common