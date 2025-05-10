#!/bin/bash
set -e

# Start GDB and connect to QEMU's GDB stub
sudo -E /home/itaymadeit/opt/cross/bin/i686-elf-gdb sysroot/boot/myos.kernel \
  -ex "target remote localhost:1234" \
  -ex "break _start"
