CFLAGS ?= -O2 -g
CPPFLAGS ?=
LDFLAGS ?=
LIBS ?=
BUILDDIR := $(BUILDDIR)/libc

DESTDIR ?=
PREFIX ?= /usr/local
EXEC_PREFIX ?= $(PREFIX)
INCLUDEDIR ?= $(PREFIX)/include
LIBDIR ?= $(EXEC_PREFIX)/lib

CFLAGS += -ffreestanding -Wall -Wextra
CPPFLAGS += -Iinclude -D__is_libc

ARCHDIR = arch/$(ARCH)

SRC_C   := $(shell find . -name "*.c" -path "./$(ARCHDIR)/*" -o -name "*.c" ! -path "./arch/*/*")
SRC_S   := $(shell find . -name "*.S" -path "./$(ARCHDIR)/*" -o -name "*.S" ! -path "./arch/*/*")
SRC_ASM := $(shell find . -name "*.asm" -path "./$(ARCHDIR)/*" -o -name "*.asm" ! -path "./arch/*/*")

SRC := $(SRC_C) $(SRC_S) $(SRC_ASM)
SRC := $(patsubst ./%, %, $(SRC))

OBJS := $(patsubst %, $(BUILDDIR)/%.o, $(basename $(SRC)))

-include $(OBJS:.o=.d)

.PHONY: all clean install install-headers install-libs
.SUFFIXES: .o .c .S .asm

all: $(BUILDDIR)/libc.a

$(BUILDDIR)/libc.a: $(OBJS)
	$(AR) rcs $@ $^

$(BUILDDIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) -MD -c $< -o $@ -std=gnu11 $(CFLAGS) $(CPPFLAGS)

$(BUILDDIR)/%.o: %.S
	@mkdir -p $(dir $@)
	$(CC) -MD -c $< -o $@ $(CFLAGS) $(CPPFLAGS)

$(BUILDDIR)/%.o: %.asm
	@mkdir -p $(dir $@)
	$(NASM) $< -o $@ $(NASMFLAGS) 


clean:
	rm -rf $(BUILDDIR)

install: install-headers install-libs

install-headers:
	mkdir -p $(DESTDIR)$(INCLUDEDIR)
	cp -R --preserve=timestamps include/. $(DESTDIR)$(INCLUDEDIR)/.

install-libs: $(BUILDDIR)/libc.a
	mkdir -p $(DESTDIR)$(LIBDIR)
	cp $(BUILDDIR)/libc.a $(DESTDIR)$(LIBDIR)

-include $(wildcard $(OBJS:.o=.d))
