DEFAULT_HOST != ../default-host.sh
HOST ?= $(DEFAULT_HOST)
HOSTARCH != ../target-triplet-to-arch.sh $(HOST)

CFLAGS ?= -O2 -g
CPPFLAGS ?=
LDFLAGS ?=
LIBS ?=

DESTDIR ?=
PREFIX ?= /usr/local
EXEC_PREFIX ?= $(PREFIX)
INCLUDEDIR ?= $(PREFIX)/include
LIBDIR ?= $(EXEC_PREFIX)/lib

CFLAGS := $(CFLAGS) -ffreestanding -Wall -Wextra
CPPFLAGS := $(CPPFLAGS) -Iinclude -I../kernel/include -D__is_libk

ARCHDIR = arch/$(HOSTARCH)

include $(ARCHDIR)/make.config

CFLAGS := $(CFLAGS) $(KERNEL_ARCH_CFLAGS)
CPPFLAGS := $(CPPFLAGS) $(KERNEL_ARCH_CPPFLAGS)

OBJS = \
  stdio/printf.o \
  stdio/putchar.o \
  stdio/puts.o \
  stdlib/abort.o \
  string/memcmp.o \
  string/memcpy.o \
  string/memmove.o \
  string/memset.o \
  string/strlen.o \
  $(ARCH_KLIB_OBJS)

.PHONY: all clean install install-headers install-libs
.SUFFIXES: .o .c .S

all: libk.a

libk.a: $(OBJS)
	$(AR) rcs $@ $^

%.o: %.c
	$(CC) -MD -c $< -o $@ -std=gnu11 $(CFLAGS) $(CPPFLAGS)

%.o: %.S
	$(CC) -MD -c $< -o $@ $(CFLAGS) $(CPPFLAGS)

clean:
	rm -f libk.a
	rm -f $(OBJS) */*.o */*/*.o
	rm -f $(OBJS:.o=.d) */*.d */*/*.d

install: install-headers install-libs

install-headers:
	@echo "No public headers to install for kernel."

install-libs: libk.a
	mkdir -p $(DESTDIR)$(LIBDIR)
	cp libk.a $(DESTDIR)$(LIBDIR)

-include $(OBJS:.o=.d)
