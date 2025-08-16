CFLAGS?=-O2 -g
CPPFLAGS?=
LDFLAGS?=
LIBS?=

DESTDIR?=
PREFIX?=/usr/local
EXEC_PREFIX?=$(PREFIX)
BOOTDIR?=$(EXEC_PREFIX)/boot
INCLUDEDIR?=$(PREFIX)/include
BUILDDIR := $(BUILDDIR)/kernel

ARCHDIR=arch/$(ARCH)
ARCHINCLUDE=arch/$(ARCH)/include

CFLAGS+= -ffreestanding -Wall -Wextra
CPPFLAGS+= -D__is_kernel -Iinclude -I../libk/include 
LDFLAGS:=$(LDFLAGS)
LIBS:=$(LIBS) -nostdlib -lk -lgcc
NASMFLAGS += -I$(ARCHINCLUDE)

CFLAGS:=$(CFLAGS) $(KERNEL_ARCH_CFLAGS)
CPPFLAGS:=$(CPPFLAGS) $(KERNEL_ARCH_CPPFLAGS)
LDFLAGS:=$(LDFLAGS) $(KERNEL_ARCH_LDFLAGS)
LIBS:=$(LIBS) $(KERNEL_ARCH_LIBS)

BARE_CC := $(firstword $(CC))

SRC_C   := $(shell find . -name "*.c" -path "./$(ARCHDIR)/*" -o -name "*.c" ! -path "./arch/*/*")
SRC_S   := $(shell find . -name "*.S" -path "./$(ARCHDIR)/*" -o -name "*.S" ! -path "./arch/*/*")
SRC_ASM := $(shell find . -name "*.asm" -path "./$(ARCHDIR)/*" -o -name "*.asm" ! -path "./arch/*/*")

SRC := $(SRC_C) $(SRC_S) $(SRC_ASM)
SRC := $(patsubst ./%, %, $(SRC))

KERNEL_OBJS := $(patsubst %, $(BUILDDIR)/%.o, $(basename $(SRC)))

OBJS = $(KERNEL_OBJS) 

LINK_LIST = $(OBJS) $(LIBS) 

.PHONY: all clean install install-headers install-kernel
.SUFFIXES: .o .c .S .asm

all: $(BUILDDIR)/waddleos.kernel

$(BUILDDIR)/waddleos.kernel: $(OBJS) $(ARCHDIR)/linker.ld
	$(CC) -T $(ARCHDIR)/linker.ld -o $@ $(CFLAGS) $(LDFLAGS) $(LINK_LIST)
	grub-file --is-x86-multiboot $(BUILDDIR)/waddleos.kernel

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

install: install-headers install-kernel

install-headers:
	@echo "No public headers to install for kernel."

install-kernel: $(BUILDDIR)/waddleos.kernel
	mkdir -p $(BOOTDIR)
	cp $(BUILDDIR)/waddleos.kernel $(BOOTDIR)

-include $(wildcard $(OBJS:.o=.d))
