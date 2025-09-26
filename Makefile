.PHONY: all clean install install-headers img run debug qemu-debug

all: install

$(info >>> Makefile is being read!)

# Core config
HOST     := $(shell ./scripts/default-host.sh)
ARCH     := $(shell ./scripts/target-triplet-to-arch.sh $(HOST))

SYSROOT   := $(shell pwd)/imgdir
DESTDIR   := $(shell pwd)/imgdir
IMGDIR    := $(shell pwd)/imgdir
BUILDTYPE ?= release
BUILDDIR  := $(shell pwd)/build/$(BUILDTYPE)
IMG       := $(shell pwd)/build/waddleos.img

PREFIX        := /root/usr
EXEC_PREFIX   := $(PREFIX)
LIBDIR        := $(EXEC_PREFIX)/lib
BOOTDIR       := $(SYSROOT)/boot
INCLUDEDIR    := $(PREFIX)/include

CFLAGS        := 
CPPFLAGS      :=
LDFLAGS       := -L$(BUILDDIR)/libk
CC            := $(HOST)-gcc --sysroot=$(SYSROOT)
AR            := $(HOST)-ar
AS            := $(HOST)-as
NASM          := nasm
NASMFLAGS     := -f elf32

ifeq ($(BUILDTYPE),debug)
CFLAGS   += -g -O0 -DDEBUG
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
	@rm -rf $(IMG) 

img: all
	@mkdir -p "$(BOOTDIR)/grub"
	@if [ ! -f "$(IMGDIR)/boot/grub/grub.cfg" ]; then \
		echo 'menuentry "Waddle-OS" {' > "$(IMGDIR)/boot/grub/grub.cfg"; \
		echo '  multiboot /waddleos.kernel' >> "$(IMGDIR)/boot/grub/grub.cfg"; \
		echo '}' >> "$(IMGDIR)/boot/grub/grub.cfg"; \
	fi
	@dd if=/dev/zero of=$(IMG) bs=1MiB count=64
	@chmod 666 /home/itaymadeit/MyFirstOS-x86/build/waddleos.img
	sudo -u builder ./scripts/img-maker.sh $(IMG)


release:
	$(MAKE) BUILDTYPE=release img

debug: 
	$(MAKE) BUILDTYPE=debug img

run: release
	qemu-system-$(ARCH) -m 256M -drive file=$(IMG),format=raw -serial stdio

debug-run: debug
	qemu-system-$(ARCH) -m 256M -drive file=$(IMG),format=raw -serial stdio \
		-s -S \
		-no-reboot -no-shutdown \
		-d int,cpu_reset -D qemu.log

debug-gdb: debug
	sudo env "PATH=$(PATH)" $(HOST)-gdb \
	  "$(SYSROOT)/boot/waddleos.kernel" \
	  -ex "target remote localhost:1234" \
	  -ex "break _start" \
	  -ex "continue"
