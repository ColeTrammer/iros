OBJS=\
$(BUILDDIR)/userland/window_server_test/main.o

EXECUTABLE=$(BUILDDIR)/userland/window_server_test/window_server_test

export LD=$(CXX)
export LDFLAGS+=-lgraphics -lws -lrt

include $(ROOT)/userland/Makefile.common