CFLAGS ?= -O2 -g
CPPFLAGS ?=
LDFLAGS ?=
LIBS ?=
BUILDDIR := $(BUILDDIR)/libk

DESTDIR ?=
PREFIX ?= /usr/local
EXEC_PREFIX ?= $(PREFIX)
INCLUDEDIR ?= $(PREFIX)/include
LIBDIR ?= $(EXEC_PREFIX)/lib

CFLAGS := $(CFLAGS) -ffreestanding -Wall -Wextra
CPPFLAGS := $(CPPFLAGS) -Iinclude -I../kernel/include -D__is_libk

ARCHDIR := arch/$(HOSTARCH)

SRC_C   := $(shell find . -name "*.c" -path "./$(ARCHDIR)/*" -o -name "*.c" ! -path "./arch/*/*")
SRC_S   := $(shell find . -name "*.S" -path "./$(ARCHDIR)/*" -o -name "*.S" ! -path "./arch/*/*")
SRC_ASM := $(shell find . -name "*.asm" -path "./$(ARCHDIR)/*" -o -name "*.asm" ! -path "./arch/*/*")

SRC := $(SRC_C) $(SRC_S) $(SRC_ASM)
SRC := $(patsubst ./%, %, $(SRC))

OBJS := $(patsubst %, $(BUILDDIR)/%.o, $(basename $(SRC)))

-include $(OBJS:.o=.d)

.PHONY: all clean install install-headers install-libs
.SUFFIXES: .o .c .S .asm

all: $(BUILDDIR)/libk.a

$(BUILDDIR)/libk.a: $(OBJS)
	$(AR) rcs $@ $^

$(BUILDDIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) -MD -c $< -o $@ -std=gnu11 $(CFLAGS) $(CPPFLAGS)

$(BUILDDIR)/%.o: %.S
	@mkdir -p $(dir $@)
	$(CC) -MD -c $< -o $@ $(CFLAGS) $(CPPFLAGS)

$(BUILDDIR)/%.o: %.asm
	@mkdir -p $(dir $@)
	$(NASM) $(NASMFLAGS) $< -o $@

clean:
	@rm -rf $(BUILDDIR)

install: install-headers install-libs

install-headers:
	@echo "No public headers to install for libk."

install-libs: $(BUILDDIR)/libk.a
	mkdir -p $(DESTDIR)$(LIBDIR)
	cp $(BUILDDIR)/libk.a $(DESTDIR)$(LIBDIR)