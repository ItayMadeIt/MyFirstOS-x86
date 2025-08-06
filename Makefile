.PHONY: all clean install install-headers iso run debug qemu-debug

all: install

$(info >>> Makefile is being read!)

# Core config
HOST     := $(shell ./default-host.sh)
ARCH     := $(shell ./target-triplet-to-arch.sh $(HOST))

SYSROOT   := $(shell pwd)/isodir
DESTDIR   := $(shell pwd)/isodir
ISODIR    := $(shell pwd)/isodir
BUILDDIR  := $(shell pwd)/build
ISO       := $(BUILDDIR)/waddleos.iso

PREFIX        := /usr
EXEC_PREFIX   := $(PREFIX)
LIBDIR        := $(EXEC_PREFIX)/lib
BOOTDIR       := $(SYSROOT)/boot
INCLUDEDIR    := $(PREFIX)/include

CFLAGS        := 
CPPFLAGS      :=
LDFLAGS       :=
CC            := $(HOST)-gcc --sysroot=$(SYSROOT)
AR            := $(HOST)-ar
AS            := $(HOST)-as
NASM          := nasm
NASMFLAGS     := -f elf32

ifeq ($(DEBUG),1)
CFLAGS   += -g -O0 -DDEBUG
CPPFLAGS += -DDEBUG
else
CFLAGS   += -O2
endif

# Workaround for -elf cross toolchains
ifeq ($(findstring -elf,$(HOST)),-elf)
  CC += -isystem=$(INCLUDEDIR)
endif

export HOST        := $(HOST)
export ARCH        := $(ARCH)
export SYSROOT     := $(SYSROOT)
export DESTDIR     := $(DESTDIR)
export BUILDDIR    := $(BUILDDIR)
export PREFIX      := $(PREFIX)
export EXEC_PREFIX := $(EXEC_PREFIX)
export BOOTDIR     := $(BOOTDIR)
export LIBDIR      := $(LIBDIR)
export INCLUDEDIR  := $(INCLUDEDIR)
export CC          := $(CC)
export CFLAGS      := $(CFLAGS)
export CPPFLAGS    := $(CPPFLAGS)
export LDFLAGS     := $(LDFLAGS)
export AR          := $(AR)
export AS          := $(AS)
export NASM        := $(NASM)
export NASMFLAGS   := $(NASMFLAGS)


SYSTEM_HEADER_PROJECTS := libc
PROJECTS := libk libc kernel

install-headers:
	@mkdir -p "$(SYSROOT)"
	@for PROJECT in $(SYSTEM_HEADER_PROJECTS); do \
		$(MAKE) -C $$PROJECT install-headers; \
	done

install: install-headers
	@for PROJECT in $(PROJECTS); do \
		$(MAKE) -C $$PROJECT install; \
	done

clean:
	$(MAKE) -C libk clean
	@for PROJECT in $(PROJECTS); do \
		echo "→ cleaning $$PROJECT"; \
		$(MAKE) -C $$PROJECT clean || exit 1; \
	done
	@rm -rf $(ISO) 

iso: install
	@mkdir -p "$(BOOTDIR)/grub"
	@if [ ! -f "$(ISODIR)/boot/grub/grub.cfg" ]; then \
		echo 'menuentry "Waddle-OS" {' > "$(ISODIR)/boot/grub/grub.cfg"; \
		echo '  multiboot /boot/waddleos.kernel' >> "$(ISODIR)/boot/grub/grub.cfg"; \
		echo '}' >> "$(ISODIR)/boot/grub/grub.cfg"; \
	fi
	grub-mkrescue -o "$(ISO)" "$(ISODIR)"

run: iso
	qemu-system-$(ARCH) -m 256M -cdrom "$(ISO)"

debug: 
	$(MAKE) DEBUG=1 iso
	sudo -E /home/itaymadeit/opt/cross/bin/$(HOST)-gdb \
	  "$(SYSROOT)/boot/waddleos.kernel" \
	  -ex "target remote localhost:1234" \
	  -ex "break entry_main" \
	  -ex "continue"

qemu-debug: 
	$(MAKE) DEBUG=1 iso
	qemu-system-$(ARCH) -cdrom "$(ISO)" -s
