OBJS=\
$(BUILDDIR)/userland/xterm/main.o \
$(BUILDDIR)/userland/xterm/tty.o \
$(BUILDDIR)/userland/xterm/vga_buffer.o

EXECUTABLE=$(BUILDDIR)/userland/xterm/xterm

CPP_FLAGS+=-fno-exceptions
export LD=$(CXX)

include $(ROOT)/userland/Makefile.common