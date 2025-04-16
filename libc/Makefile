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
CPPFLAGS := $(CPPFLAGS) -Iinclude -D__is_libc

ARCHDIR = arch/$(HOSTARCH)

include $(ARCHDIR)/make.config

CFLAGS := $(CFLAGS) $(ARCH_CFLAGS)
CPPFLAGS := $(CPPFLAGS) $(ARCH_CPPFLAGS)

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
  $(ARCH_HOSTEDOBJS)

.PHONY: all clean install install-headers install-libs
.SUFFIXES: .o .c .S

all: libc.a

libc.a: $(OBJS)
	$(AR) rcs $@ $^

%.o: %.c
	$(CC) -MD -c $< -o $@ -std=gnu11 $(CFLAGS) $(CPPFLAGS)

%.o: %.S
	$(CC) -MD -c $< -o $@ $(CFLAGS) $(CPPFLAGS)

clean:
	rm -f libc.a
	rm -f $(OBJS) */*.o */*/*.o
	rm -f $(OBJS:.o=.d) */*.d */*/*.d

install: install-headers install-libs

install-headers:
	mkdir -p $(DESTDIR)$(INCLUDEDIR)
	cp -R --preserve=timestamps include/. $(DESTDIR)$(INCLUDEDIR)/.

install-libs: libc.a
	mkdir -p $(DESTDIR)$(LIBDIR)
	cp libc.a $(DESTDIR)$(LIBDIR)

-include $(OBJS:.o=.d)
