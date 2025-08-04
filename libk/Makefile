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

ARCHDIR := arch/$(HOSTARCH)

-include $(ARCHDIR)/make.config

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
	@echo "=== CLEAN TARGET TRIGGERED in $(notdir $(CURDIR)) ==="
	@echo "→ cleaning libk from $(PWD)"
	@echo "→ found:"
	@find . -name '*.o' -print
	@find . -name '*.d' -print
	@echo "→ deleting:"
	@find . -name '*.o' -delete -print
	@find . -name '*.d' -delete -print
	@rm -v -f libk.a

install: install-headers install-libs

install-headers:
	@echo "No public headers to install for kernel."

install-libs: libk.a
	mkdir -p $(DESTDIR)$(LIBDIR)
	cp libk.a $(DESTDIR)$(LIBDIR)

-include $(wildcard $(OBJS:.o=.d))
