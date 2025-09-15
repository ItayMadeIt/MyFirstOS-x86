#!/bin/sh
set -e

# Image has that: empty 64 MB 
# dd if=/dev/zero of=$IMAGE bs=1MiB count=64
IMAGE=$1

if [ -z "$IMAGE" ]; then
  echo "Usage: $0 <image-file>"
  exit 1
fi

#config
MNT=build/mnt
MNT_BOOT=$MNT/boot
MNT_ROOT=$MNT/root
STAGE_BOOT=imgdir/boot
STAGE_ROOT=imgdir/root


cleanup() 
{
    echo "[*] Cleaning up..."
    sudo umount -l "$MNT_BOOT" 2>/dev/null || true
    sudo umount -l "$MNT_ROOT" 2>/dev/null || true
    sudo losetup -D 2>/dev/null || true
    sudo rm -rf "$MNT"
}
trap cleanup EXIT

# Make 2 partitions in MBR style
parted -s $IMAGE mklabel msdos
parted -s $IMAGE mkpart primary fat16 1MiB 20MiB
parted -s $IMAGE mkpart primary ext2 20MiB 100%

# Make the parititons link into a loop dev
LOOP=$(losetup -f --show -P $IMAGE)

# Format both partitions into their respective filesystems
sudo mkfs.fat -F16 ${LOOP}p1
sudo mkfs.ext2 -F ${LOOP}p2

# Mount loop dev into actual mnt folders
sudo mkdir -p $MNT_BOOT $MNT_ROOT
sudo mount ${LOOP}p1 $MNT_BOOT
sudo mount ${LOOP}p2 $MNT_ROOT

# Copy the files into their mnt relatives
sudo cp -r $STAGE_BOOT/* $MNT_BOOT/ || true
sudo cp -r $STAGE_ROOT/* $MNT_ROOT/ || true

# Install grub on that
sudo grub-install \
  --target=i386-pc \
  --boot-directory=$MNT_BOOT \
  --modules="normal multiboot" \
  $LOOP
